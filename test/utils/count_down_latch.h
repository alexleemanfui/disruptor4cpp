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

#ifndef DISRUPTOR4CPP_TEST_UTILS_COUNT_DOWN_LATCH_H_
#define DISRUPTOR4CPP_TEST_UTILS_COUNT_DOWN_LATCH_H_

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace disruptor4cpp
{
	namespace test
	{
		class count_down_latch
		{
		public:
			explicit count_down_latch(int count)
				: count_(count)
			{
			}

			~count_down_latch() = default;

			void wait()
			{
				std::unique_lock<std::mutex> lock(mutex_);
				condition_.wait(lock, [this] { return count_ == 0; });
			}

			// Return true if the count reached zero
			// and false if the waiting time elapsed before the count reached zero.
			template< class Rep, class Period>
			bool wait(const std::chrono::duration<Rep, Period>& timeout)
			{
				std::unique_lock<std::mutex> lock(mutex_);
				return condition_.wait_for(lock, timeout, [this] { return count_ <= 0; });
			}

			void count_down()
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (--count_ <= 0)
					condition_.notify_all();
			}

		private:
			count_down_latch(const count_down_latch&) = delete;
			count_down_latch& operator=(const count_down_latch&) = delete;
			count_down_latch(count_down_latch&&) = delete;
			count_down_latch& operator=(count_down_latch&&) = delete;

			std::mutex mutex_;
			std::condition_variable condition_;
			int count_;
		};
	}
}

#endif
