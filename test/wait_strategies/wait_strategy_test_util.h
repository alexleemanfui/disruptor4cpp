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

#ifndef DISRUPTOR4CPP_TEST_WAIT_STRATEGIES_WAIT_STRATEGY_TEST_UTIL_H_
#define DISRUPTOR4CPP_TEST_WAIT_STRATEGIES_WAIT_STRATEGY_TEST_UTIL_H_

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <disruptor4cpp/disruptor4cpp.h>
#include "sequence_updater.h"
#include "../dummy_sequence_barrier.h"

namespace disruptor4cpp
{
	namespace test
	{
		class wait_strategy_test_util
		{
		public:
			template <class Rep, class Period, typename TWaitStrategy>
			static void assert_wait_for_with_delay_of(const std::chrono::duration<Rep, Period>& sleep_time,
				TWaitStrategy& wait_strategy)
			{
				sequence_updater<Rep, Period, TWaitStrategy> seq_updater(sleep_time, wait_strategy);
				std::thread t([&seq_updater] { seq_updater.run(); });
				seq_updater.wait_for_startup();

				sequence cursor(0);
				dummy_sequence_barrier seq_barrier;
				auto dependent_sequences = fixed_sequence_group<sequence>::create(seq_updater.get_sequence());
				int64_t seq = wait_strategy.wait_for(0, cursor, dependent_sequences, seq_barrier);
				ASSERT_EQ(0, seq);

				t.join();
			}
		};
	}
}

#endif
