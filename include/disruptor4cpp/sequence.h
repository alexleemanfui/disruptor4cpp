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

#ifndef DISRUPTOR4CPP_SEQUENCE_H_
#define DISRUPTOR4CPP_SEQUENCE_H_

#include <atomic>
#include <cstdint>

#include "utils/cache_line_storage.h"

namespace disruptor4cpp
{
	class sequence
	{
	public:
		static constexpr int64_t INITIAL_VALUE = -1;

		sequence()
			: sequence_(INITIAL_VALUE)
		{
		}

		explicit sequence(int64_t initial_value)
			: sequence_(initial_value)
		{
		}

		~sequence() = default;

		int64_t get() const
		{
			return sequence_.load(std::memory_order_acquire);
		}

		void set(int64_t value)
		{
			sequence_.store(value, std::memory_order_release);
		}

		bool compare_and_set(int64_t expected_value, int64_t new_value)
		{
			return sequence_.compare_exchange_weak(expected_value, new_value);
		}

		int64_t increment_and_get()
		{
			return add_and_get(1);
		}

		int64_t add_and_get(int64_t increment)
		{
			return sequence_.fetch_add(increment, std::memory_order_release) + increment;
		}

	private:
		sequence(const sequence&) = delete;
		sequence& operator=(const sequence&) = delete;
		sequence(sequence&&) = delete;
		sequence& operator=(sequence&&) = delete;

		alignas(CACHE_LINE_SIZE) std::atomic<int64_t> sequence_;
		char padding[CACHE_LINE_SIZE - sizeof(std::atomic<int64_t>)];
	};
}

#endif
