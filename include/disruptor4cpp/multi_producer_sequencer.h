/*
Copyright (c) 2015, Alex Man-fui Lee
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of disruptor4cpp nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DISRUPTOR4CPP_MULTI_PRODUCER_SEQUENCER_H_
#define DISRUPTOR4CPP_MULTI_PRODUCER_SEQUENCER_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "exceptions/insufficient_capacity_exception.h"
#include "sequence.h"
#include "sequence_barrier.h"
#include "utils/util.h"

namespace disruptor4cpp
{
	template <std::size_t BufferSize, typename TWaitStrategy, typename TSequence = sequence>
	class multi_producer_sequencer
	{
	public:
		typedef TWaitStrategy wait_strategy_type;
		typedef TSequence sequence_type;
		typedef sequence_barrier<
			multi_producer_sequencer<BufferSize, TWaitStrategy, TSequence>> sequence_barrier_type;

		static constexpr int64_t INITIAL_CURSOR_VALUE = -1;
		static constexpr std::size_t BUFFER_SIZE = BufferSize;

		multi_producer_sequencer()
			: cursor_(),
			  gating_sequence_cache_(),
			  wait_strategy_(),
			  gating_sequences_()
		{
			std::memset(available_buffer_, ~0, sizeof(available_buffer_));
		}

		~multi_producer_sequencer() = default;

		int64_t get_cursor() const
		{
			return cursor_.get();
		}

		constexpr std::size_t get_buffer_size() const
		{
			return BUFFER_SIZE;
		}

		TWaitStrategy& get_wait_strategy()
		{
			return wait_strategy_;
		}

		void add_gating_sequences(const std::vector<TSequence*>& sequences_to_add)
		{
			// TODO: It is different from the java version.
			// The java version allows to change gating sequences after disruptor is started.
			int64_t cursor_sequence = cursor_.get();
			for (auto seq : sequences_to_add)
			{
				seq->set(cursor_sequence);
				gating_sequences_.push_back(seq);
			}
		}

		bool remove_gating_sequence(const TSequence& seq)
		{
			// TODO: It is different from the java version.
			// The java version allows to change gating sequences after disruptor is started.
			bool removed = false;
			for (auto iter = gating_sequences_.begin(); iter != gating_sequences_.end();)
			{
				if (*iter == &seq)
				{
					removed = true;
					iter = gating_sequences_.erase(iter);
				}
				else
					++iter;
			}
			return removed;
		}

		int64_t get_minimum_sequence() const
		{
			return util::get_minimum_sequence(gating_sequences_);
		}

		std::unique_ptr<sequence_barrier_type> new_barrier()
		{
			return new_barrier(std::vector<TSequence*>());
		}

		std::unique_ptr<sequence_barrier_type> new_barrier(const std::vector<TSequence*>& sequences_to_track)
		{
			return std::unique_ptr<sequence_barrier_type>(
				new sequence_barrier_type(*this, wait_strategy_, cursor_, sequences_to_track));
		}

		bool has_available_capacity(int required_capacity)
		{
			return has_available_capacity(required_capacity, cursor_.get());
		}

		int64_t next()
		{
			return next(1);
		}

		int64_t next(int n)
		{
			if (n < 1)
				throw std::invalid_argument("n must be > 0");

			int64_t current;
			int64_t next;
			do
			{
				current = cursor_.get();
				next = current + n;

				int64_t wrap_point = next - BufferSize;
				int64_t cached_gating_sequence = gating_sequence_cache_.get();
				if (wrap_point > cached_gating_sequence || cached_gating_sequence > current)
				{
					int64_t gating_sequence = util::get_minimum_sequence(gating_sequences_, current);
					if (wrap_point > gating_sequence)
					{
						std::this_thread::yield();
						continue;
					}
					gating_sequence_cache_.set(gating_sequence);
				}
				else if (cursor_.compare_and_set(current, next))
					break;
			}
			while (true);
			return next;
		}

		int64_t try_next()
		{
			return try_next(1);
		}

		int64_t try_next(int n)
		{
			if (n < 1)
				throw std::invalid_argument("n must be > 0");

			int64_t current;
			int64_t next;
			do
			{
				current = cursor_.get();
				next = current + n;
				if (!has_available_capacity(n, current))
					throw insufficient_capacity_exception();
			}
			while (!cursor_.compare_and_set(current, next));
			return next;
		}

		int64_t remaining_capacity() const
		{
			int64_t consumed = util::get_minimum_sequence(gating_sequences_, cursor_.get());
			int64_t produced = cursor_.get();
			return BufferSize - (produced - consumed);
		}

		void claim(int64_t seq)
		{
			cursor_.set(seq);
		}

		void publish(int64_t seq)
		{
			set_available(seq);
			wait_strategy_.signal_all_when_blocking();
		}

		void publish(int64_t lo, int64_t hi)
		{
			for (int64_t i = lo; i <= hi; i++)
			{
				set_available(i);
			}
			wait_strategy_.signal_all_when_blocking();
		}

		bool is_available(int64_t seq) const
		{
			int index = calculate_index(seq);
			int flag = calculate_availability_flag(seq);
			return available_buffer_[index] == flag;
		}

		int64_t get_highest_published_sequence(int64_t lower_bound, int64_t available_sequence) const
		{
			for (int64_t seq = lower_bound; seq <= available_sequence; seq++)
			{
				if (!is_available(seq))
					return seq - 1;
			}
			return available_sequence;
		}

	private:
		multi_producer_sequencer(const multi_producer_sequencer&) = delete;
		multi_producer_sequencer& operator=(const multi_producer_sequencer&) = delete;
		multi_producer_sequencer(multi_producer_sequencer&&) = delete;
		multi_producer_sequencer& operator=(multi_producer_sequencer&&) = delete;

		static constexpr int INDEX_MASK = BufferSize - 1;
		static constexpr int INDEX_SHIFT = util::log2(BufferSize);

		bool has_available_capacity(int required_capacity, int cursor_value)
		{
			int64_t wrap_point = (cursor_value + required_capacity) - BufferSize;
			int64_t cached_gating_sequence = gating_sequence_cache_.get();
			if (wrap_point > cached_gating_sequence || cached_gating_sequence > cursor_value)
			{
				int64_t min_sequence = util::get_minimum_sequence(gating_sequences_, cursor_value);
				gating_sequence_cache_.set(min_sequence);
				if (wrap_point > min_sequence)
					return false;
			}
			return true;
		}

		void set_available(int64_t seq)
		{
			int index = calculate_index(seq);
			int flag = calculate_availability_flag(seq);
			available_buffer_[index] = flag;
		}

		int calculate_availability_flag(int64_t seq) const
		{
			return (int)(((uint64_t)seq) >> INDEX_SHIFT);
		}

		int calculate_index(int64_t seq) const
		{
			return ((int)seq) & INDEX_MASK;
		}

		TSequence cursor_;
		TSequence gating_sequence_cache_;
		TWaitStrategy wait_strategy_;
		std::vector<TSequence*> gating_sequences_;
		int available_buffer_[BufferSize];
	};
}

#endif
