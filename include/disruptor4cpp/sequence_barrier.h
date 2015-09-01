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

#ifndef DISRUPTOR4CPP_SEQUENCE_BARRIER_H_
#define DISRUPTOR4CPP_SEQUENCE_BARRIER_H_

#include <atomic>
#include <vector>

#include "exceptions/alert_exception.h"
#include "fixed_sequence_group.h"
#include "sequence.h"
#include "utils/util.h"

namespace disruptor4cpp
{
	template <typename TSequencer>
	class sequence_barrier
	{
	public:
		typedef TSequencer sequencer_type;
		typedef typename TSequencer::wait_strategy_type wait_strategy_type;
		typedef typename TSequencer::sequence_type sequence_type;

		sequence_barrier(const TSequencer& sequencer,
			wait_strategy_type& wait_strategy,
			const sequence_type& cursor_sequence,
			const std::vector<sequence_type*> dependent_sequences)
			: dependent_sequence_(dependent_sequences.empty()
				? fixed_sequence_group<sequence_type>::create(cursor_sequence)
				: fixed_sequence_group<sequence_type>::create(dependent_sequences)),
			  sequencer_(sequencer),
			  wait_strategy_(wait_strategy),
			  cursor_sequence_(cursor_sequence),
			  alerted_(false)
		{
		}

		~sequence_barrier() = default;

		int64_t wait_for(int64_t seq)
		{
			check_alert();

			int64_t available_sequence = wait_strategy_.wait_for(seq, cursor_sequence_, dependent_sequence_, *this);
			if (available_sequence < seq)
				return available_sequence;
			return sequencer_.get_highest_published_sequence(seq, available_sequence);
		}

		int64_t get_cursor() const
		{
			return dependent_sequence_.get();
		}

		bool is_alerted() const
		{
			return alerted_.load(std::memory_order_acquire);
		}

		void alert()
		{
			alerted_.store(true, std::memory_order_release);
			wait_strategy_.signal_all_when_blocking();
		}

		void clear_alert()
		{
			alerted_.store(false, std::memory_order_release);
		}

		void check_alert() const
		{
			if (is_alerted())
				throw alert_exception();
		}

	private:
		sequence_barrier(const sequence_barrier&) = delete;
		sequence_barrier& operator=(const sequence_barrier&) = delete;
		sequence_barrier(sequence_barrier&&) = delete;
		sequence_barrier& operator=(sequence_barrier&&) = delete;

		fixed_sequence_group<sequence_type> dependent_sequence_;
		const TSequencer& sequencer_;
		wait_strategy_type& wait_strategy_;
		const sequence_type& cursor_sequence_;
		std::atomic<bool> alerted_;
	};
}

#endif
