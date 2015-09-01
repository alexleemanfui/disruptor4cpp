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

#ifndef DISRUPTOR4CPP_WAIT_STRATEGIES_PHASED_BACKOFF_WAIT_STRATEGY_H_
#define DISRUPTOR4CPP_WAIT_STRATEGIES_PHASED_BACKOFF_WAIT_STRATEGY_H_

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

#include "../fixed_sequence_group.h"

namespace disruptor4cpp
{
	template <int64_t SpinTimeoutNanoseconds, int64_t YieldTimeoutNanoseconds,
		typename TFallbackStrategy>
	class phased_backoff_wait_strategy
	{
	public:
		phased_backoff_wait_strategy() = default;
		~phased_backoff_wait_strategy() = default;

		template <typename TSequenceBarrier, typename TSequence>
		int64_t wait_for(int64_t seq, const TSequence& cursor_sequence,
			const fixed_sequence_group<TSequence>& dependent_sequence,
			const TSequenceBarrier& seq_barrier)
		{
			int64_t available_sequence = 0;
			std::chrono::system_clock::time_point start_time;
			int counter = SPIN_TRIES;

			do
			{
				if ((available_sequence = dependent_sequence.get()) >= seq)
					return available_sequence;

				--counter;
				if (counter == 0)
				{
					if (start_time.time_since_epoch() == std::chrono::system_clock::duration::zero())
						start_time = std::chrono::system_clock::now();
					else
					{
						int64_t time_delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
							std::chrono::system_clock::now() - start_time).count();
						if (time_delta > YieldTimeoutNanoseconds)
							return fallback_strategy_.wait_for(seq, cursor_sequence, dependent_sequence, seq_barrier);
						else if (time_delta > SpinTimeoutNanoseconds)
							std::this_thread::yield();
					}
					counter = SPIN_TRIES;
				}
			}
			while (true);
		}

		void signal_all_when_blocking()
		{
			fallback_strategy_.signal_all_when_blocking();
		}

	private:
		phased_backoff_wait_strategy(const phased_backoff_wait_strategy&) = delete;
		phased_backoff_wait_strategy& operator=(const phased_backoff_wait_strategy&) = delete;
		phased_backoff_wait_strategy(phased_backoff_wait_strategy&&) = delete;
		phased_backoff_wait_strategy& operator=(phased_backoff_wait_strategy&&) = delete;

		static constexpr int SPIN_TRIES = 10000;

		TFallbackStrategy fallback_strategy_;
	};
}

#endif
