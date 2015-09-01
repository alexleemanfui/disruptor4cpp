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

#ifndef DISRUPTOR4CPP_WAIT_STRATEGIES_YIELDING_WAIT_STRATEGY_H_
#define DISRUPTOR4CPP_WAIT_STRATEGIES_YIELDING_WAIT_STRATEGY_H_

#include <cstdint>
#include <thread>

#include "../fixed_sequence_group.h"

namespace disruptor4cpp
{
	template <int SpinTries = 100>
	class yielding_wait_strategy
	{
	public:
		yielding_wait_strategy() = default;
		~yielding_wait_strategy() = default;

		template <typename TSequenceBarrier, typename TSequence>
		int64_t wait_for(int64_t seq, const TSequence& cursor_sequence,
			const fixed_sequence_group<TSequence>& dependent_sequence,
			const TSequenceBarrier& seq_barrier)
		{
			int64_t available_sequence = 0;
			int counter = SpinTries;

			while ((available_sequence = dependent_sequence.get()) < seq)
			{
				counter = apply_wait_method(seq_barrier, counter);
			}
			return available_sequence;
		}

		void signal_all_when_blocking()
		{
		}

	private:
		yielding_wait_strategy(const yielding_wait_strategy&) = delete;
		yielding_wait_strategy& operator=(const yielding_wait_strategy&) = delete;
		yielding_wait_strategy(yielding_wait_strategy&&) = delete;
		yielding_wait_strategy& operator=(yielding_wait_strategy&&) = delete;

		template <typename TSequenceBarrier>
		int apply_wait_method(const TSequenceBarrier& seq_barrier, int counter)
		{
			seq_barrier.check_alert();
			if (counter == 0)
				std::this_thread::yield();
			else
				--counter;
			return counter;
		}
	};
}

#endif
