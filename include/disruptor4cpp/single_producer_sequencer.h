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

#ifndef DISRUPTOR4CPP_SINGLE_PRODUCER_SEQUENCER_H_
#define DISRUPTOR4CPP_SINGLE_PRODUCER_SEQUENCER_H_

#include <cstddef>
#include <cstdint>
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
	class single_producer_sequencer
	{
	public:
		typedef TWaitStrategy wait_strategy_type;
		typedef TSequence sequence_type;
		typedef sequence_barrier<
			single_producer_sequencer<BufferSize, TWaitStrategy, TSequence>> sequence_barrier_type;

		static constexpr int64_t INITIAL_CURSOR_VALUE = -1;
		static constexpr std::size_t BUFFER_SIZE = BufferSize;

		single_producer_sequencer()
			: cursor_(),
			  wait_strategy_(),
			  gating_sequences_(),
			  next_value_(TSequence::INITIAL_VALUE),
			  cached_value_(TSequence::INITIAL_VALUE)
		{
		}

		~single_producer_sequencer() = default;

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
			int64_t next_value = next_value_;
			int64_t wrap_point = (next_value + required_capacity) - BufferSize;
			int64_t cached_gating_sequence = cached_value_;
			if (wrap_point > cached_gating_sequence || cached_gating_sequence > next_value)
			{
				int64_t min_sequence = util::get_minimum_sequence(gating_sequences_, next_value);
				cached_value_ = min_sequence;
				if (wrap_point > min_sequence)
					return false;
			}
			return true;
		}

		int64_t next()
		{
			return next(1);
		}

		int64_t next(int n)
		{
			if (n < 1)
				throw std::invalid_argument("n must be > 0");

			int64_t next_value = next_value_;
			int64_t next_sequence = next_value + n;
			int64_t wrap_point = next_sequence - BufferSize;
			int64_t cached_gating_sequence = cached_value_;

			if (wrap_point > cached_gating_sequence || cached_gating_sequence > next_value)
			{
				int64_t min_sequence;
				while (wrap_point > (min_sequence = util::get_minimum_sequence(gating_sequences_, next_value)))
				{
					std::this_thread::yield();
				}
				cached_value_ = min_sequence;
			}
			next_value_ = next_sequence;
			return next_sequence;
		}

		int64_t try_next()
		{
			return try_next(1);
		}

		int64_t try_next(int n)
		{
			if (n < 1)
				throw std::invalid_argument("n must be > 0");
			if (!has_available_capacity(n))
				throw insufficient_capacity_exception();

			int64_t next_sequence = (next_value_ += n);
			return next_sequence;
		}

		int64_t remaining_capacity() const
		{
			int64_t next_value = next_value_;
			int64_t consumed = util::get_minimum_sequence(gating_sequences_, next_value);
			int64_t produced = next_value;
			return BufferSize - (produced - consumed);
		}

		void claim(int64_t seq)
		{
			next_value_ = seq;
		}

		void publish(int64_t seq)
		{
			cursor_.set(seq);
			wait_strategy_.signal_all_when_blocking();
		}

		void publish(int64_t lo, int64_t hi)
		{
			publish(hi);
		}

		bool is_available(int64_t seq) const
		{
			return seq <= cursor_.get();
		}

		int64_t get_highest_published_sequence(int64_t lower_bound, int64_t available_sequence) const
		{
			return available_sequence;
		}

	private:
		single_producer_sequencer(const single_producer_sequencer&) = delete;
		single_producer_sequencer& operator=(const single_producer_sequencer&) = delete;
		single_producer_sequencer(single_producer_sequencer&&) = delete;
		single_producer_sequencer& operator=(single_producer_sequencer&&) = delete;

		TSequence cursor_;
		TWaitStrategy wait_strategy_;
		std::vector<TSequence*> gating_sequences_;
		int64_t next_value_;
		int64_t cached_value_;
	};
}

#endif
