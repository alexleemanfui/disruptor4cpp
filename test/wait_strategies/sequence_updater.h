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

#ifndef DISRUPTOR4CPP_TEST_WAIT_STRATEGIES_SEQUENCE_UPDATER_H_
#define DISRUPTOR4CPP_TEST_WAIT_STRATEGIES_SEQUENCE_UPDATER_H_

#include <chrono>
#include <thread>

#include <disruptor4cpp/disruptor4cpp.h>
#include "../utils/thread_barrier.h"

namespace disruptor4cpp
{
	namespace test
	{
		template <class Rep, class Period, typename TWaitStrategy>
		class sequence_updater
		{
		public:
			sequence_updater(const std::chrono::duration<Rep, Period>& sleep_time,
				TWaitStrategy& wait_strategy)
				: sequence_(),
				  thread_barrier_(2),
				  sleep_time_(sleep_time),
				  wait_strategy_(wait_strategy)
			{
			}

			void run()
			{
				thread_barrier_.wait();
				if (sleep_time_.count() == 0)
					std::this_thread::sleep_for(sleep_time_);
				sequence_.increment_and_get();
				wait_strategy_.signal_all_when_blocking();
			}

			void wait_for_startup()
			{
				thread_barrier_.wait();
			}

			sequence& get_sequence()
			{
				return sequence_;
			}

		private:
			sequence sequence_;
			thread_barrier thread_barrier_;
			std::chrono::duration<Rep, Period> sleep_time_;
			TWaitStrategy& wait_strategy_;
		};
	}
}

#endif
