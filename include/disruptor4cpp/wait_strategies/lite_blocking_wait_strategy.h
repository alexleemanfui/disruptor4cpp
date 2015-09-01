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

#ifndef DISRUPTOR4CPP_WAIT_STRATEGIES_LITE_BLOCKING_WAIT_STRATEGY_H_
#define DISRUPTOR4CPP_WAIT_STRATEGIES_LITE_BLOCKING_WAIT_STRATEGY_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "../fixed_sequence_group.h"

namespace disruptor4cpp
{
	class lite_blocking_wait_strategy
	{
	public:
		lite_blocking_wait_strategy()
			: signal_needed_(false)
		{
		}

		~lite_blocking_wait_strategy() = default;

		template <typename TSequenceBarrier, typename TSequence>
		int64_t wait_for(int64_t seq, const TSequence& cursor_sequence,
			const fixed_sequence_group<TSequence>& dependent_sequence,
			const TSequenceBarrier& seq_barrier)
		{
			int64_t available_sequence = 0;
			if ((available_sequence = cursor_sequence.get()) < seq)
			{
				std::unique_lock<std::recursive_mutex> lock(mutex_);
				do
				{
					signal_needed_.store(true, std::memory_order_release);
					if ((available_sequence = cursor_sequence.get()) >= seq)
						break;

					seq_barrier.check_alert();
					processor_notify_condition_.wait(lock);
				}
				while ((available_sequence = cursor_sequence.get()) < seq);
			}

			while ((available_sequence = dependent_sequence.get()) < seq)
			{
				seq_barrier.check_alert();
			}
			return available_sequence;
		}

		void signal_all_when_blocking()
		{
			if (signal_needed_.exchange(false, std::memory_order_release))
			{
				std::lock_guard<std::recursive_mutex> lock(mutex_);
				processor_notify_condition_.notify_all();
			}
		}

	private:
		lite_blocking_wait_strategy(const lite_blocking_wait_strategy&) = delete;
		lite_blocking_wait_strategy& operator=(const lite_blocking_wait_strategy&) = delete;
		lite_blocking_wait_strategy(lite_blocking_wait_strategy&&) = delete;
		lite_blocking_wait_strategy& operator=(lite_blocking_wait_strategy&&) = delete;

		std::recursive_mutex mutex_;
		std::condition_variable_any processor_notify_condition_;
		std::atomic<bool> signal_needed_;
	};
}

#endif
