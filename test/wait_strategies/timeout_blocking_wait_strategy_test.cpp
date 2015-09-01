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

#include <chrono>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <disruptor4cpp/disruptor4cpp.h>
#include "wait_strategy_test_util.h"

namespace
{
	class mock_sequence_barrier
	{
	public:
		MOCK_CONST_METHOD0(check_alert, void());
	};
}

namespace disruptor4cpp
{
	namespace test
	{
		TEST(timeout_blocking_wait_strategy_test, should_timeout_wait_for)
		{
			const int64_t timeout_nanos = 500 * 1000000;
			timeout_blocking_wait_strategy<timeout_nanos> wait_strategy;
			sequence cursor(5);
			auto dependent_sequences = fixed_sequence_group<sequence>::create(std::vector<sequence*> { &cursor });

			mock_sequence_barrier seq_barrier;
			EXPECT_CALL(seq_barrier, check_alert());

			auto t0 = std::chrono::system_clock::now();
			try
			{
				wait_strategy.wait_for(6, cursor, dependent_sequences, seq_barrier);
			}
			catch (timeout_exception& ex)
			{
			}
			auto t1 = std::chrono::system_clock::now();
			int64_t time_waiting = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
			ASSERT_GE(time_waiting, timeout_nanos);
		}
	}
}
