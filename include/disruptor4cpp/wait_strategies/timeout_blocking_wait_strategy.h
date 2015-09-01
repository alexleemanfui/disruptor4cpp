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

#ifndef DISRUPTOR4CPP_WAIT_STRATEGIES_TIMEOUT_BLOCKING_WAIT_STRATEGY_H_
#define DISRUPTOR4CPP_WAIT_STRATEGIES_TIMEOUT_BLOCKING_WAIT_STRATEGY_H_

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "../exceptions/timeout_exception.h"
#include "../fixed_sequence_group.h"

namespace disruptor4cpp
{
	template <int64_t TimeoutNanoseconds>
	class timeout_blocking_wait_strategy
	{
	public:
		timeout_blocking_wait_strategy() = default;
		~timeout_blocking_wait_strategy() = default;

		template <typename TSequenceBarrier, typename TSequence>
		int64_t wait_for(int64_t seq, const TSequence& cursor_sequence,
			const fixed_sequence_group<TSequence>& dependent_sequence,
			const TSequenceBarrier& seq_barrier)
		{
			int64_t available_sequence = 0;
			if ((available_sequence = cursor_sequence.get()) < seq)
			{
				std::unique_lock<std::recursive_mutex> lock(mutex_);
				while ((available_sequence = cursor_sequence.get()) < seq)
				{
					seq_barrier.check_alert();
					std::cv_status status = processor_notify_condition_.wait_for(lock,
						std::chrono::nanoseconds(TimeoutNanoseconds));
					if (status == std::cv_status::timeout)
						throw timeout_exception();
				}
			}

			while ((available_sequence = dependent_sequence.get()) < seq)
			{
				seq_barrier.check_alert();
			}
			return available_sequence;
		}

		void signal_all_when_blocking()
		{
			std::lock_guard<std::recursive_mutex> lock(mutex_);
			processor_notify_condition_.notify_all();
		}

	private:
		timeout_blocking_wait_strategy(const timeout_blocking_wait_strategy&) = delete;
		timeout_blocking_wait_strategy& operator=(const timeout_blocking_wait_strategy&) = delete;
		timeout_blocking_wait_strategy(timeout_blocking_wait_strategy&&) = delete;
		timeout_blocking_wait_strategy& operator=(timeout_blocking_wait_strategy&&) = delete;

		std::recursive_mutex mutex_;
		std::condition_variable_any processor_notify_condition_;
	};
}

#endif
