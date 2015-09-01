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

#ifndef DISRUPTOR4CPP_TEST_UTILS_THREAD_BARRIER_H_
#define DISRUPTOR4CPP_TEST_UTILS_THREAD_BARRIER_H_

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace disruptor4cpp
{
	namespace test
	{
		class thread_barrier
		{
		public:
			explicit thread_barrier(std::size_t count)
				: total_wait_count_(count),
				  current_wait_count_(count),
				  generation_(0)
			{
			}

			~thread_barrier() = default;

			void wait()
			{
				std::size_t gen = generation_;
				std::unique_lock<std::mutex> lock(mutex_);
				if (--current_wait_count_ == 0)
				{
					generation_++;
					current_wait_count_ = total_wait_count_;
					condition_.notify_all();
				}
				else
					condition_.wait(lock, [this, gen] { return gen != generation_; });
			}

		private:
			thread_barrier(const thread_barrier&) = delete;
			thread_barrier& operator=(const thread_barrier&) = delete;
			thread_barrier(thread_barrier&&) = delete;
			thread_barrier& operator=(thread_barrier&&) = delete;

			std::mutex mutex_;
			std::condition_variable condition_;
			std::size_t total_wait_count_;
			std::size_t current_wait_count_;
			std::size_t generation_;
		};
	}
}

#endif
