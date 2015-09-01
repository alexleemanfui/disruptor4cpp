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

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <disruptor4cpp/disruptor4cpp.h>

namespace disruptor4cpp
{
	namespace test
	{
		class mock_wait_strategy
		{
		public:
			MOCK_CONST_METHOD0(signal_all_when_blocking, void());
		};

		struct producer_type_single
		{
			static constexpr producer_type value = producer_type::single;
		};

		struct producer_type_multi
		{
			static constexpr producer_type value = producer_type::multi;
		};

		template <typename TProducerType>
		class sequencer_test : public testing::Test
		{
		protected:
			static constexpr int BUFFER_SIZE = 16;

			typename sequencer_traits<BUFFER_SIZE, blocking_wait_strategy,
				sequence, TProducerType::value>::sequencer_type sequencer_;
			typename sequencer_traits<BUFFER_SIZE, testing::NiceMock<mock_wait_strategy>,
				sequence, TProducerType::value>::sequencer_type mock_sequencer_;
			sequence gating_sequence_;
		};

		typedef ::testing::Types<producer_type_single, producer_type_multi> producer_types;
		TYPED_TEST_CASE(sequencer_test, producer_types);

		TYPED_TEST(sequencer_test, should_start_with_initial_value)
		{
			ASSERT_EQ(0, this->sequencer_.next());
		}

		TYPED_TEST(sequencer_test, should_batch_claim)
		{
			ASSERT_EQ(3, this->sequencer_.next(4));
		}

		TYPED_TEST(sequencer_test, should_indicate_has_available_capacity)
		{
			this->sequencer_.add_gating_sequences(std::vector<sequence*> { &this->gating_sequence_ });

			ASSERT_TRUE(this->sequencer_.has_available_capacity(1));
			ASSERT_TRUE(this->sequencer_.has_available_capacity(this->BUFFER_SIZE));
			ASSERT_FALSE(this->sequencer_.has_available_capacity(this->BUFFER_SIZE + 1));

			this->sequencer_.publish(this->sequencer_.next());
			ASSERT_TRUE(this->sequencer_.has_available_capacity(this->BUFFER_SIZE - 1));
			ASSERT_FALSE(this->sequencer_.has_available_capacity(this->BUFFER_SIZE));
		}

		TYPED_TEST(sequencer_test, should_indicate_no_available_capacity)
		{
			this->sequencer_.add_gating_sequences(std::vector<sequence*> { &this->gating_sequence_ });
			int64_t seq = this->sequencer_.next(this->BUFFER_SIZE);
			this->sequencer_.publish(seq - (this->BUFFER_SIZE - 1), seq);

			ASSERT_FALSE(this->sequencer_.has_available_capacity(1));
		}

		TYPED_TEST(sequencer_test, should_hold_up_publisher_when_buffer_is_full)
		{
			this->sequencer_.add_gating_sequences(std::vector<sequence*> { &this->gating_sequence_ });
			int64_t seq = this->sequencer_.next(this->BUFFER_SIZE);
			this->sequencer_.publish(seq - (this->BUFFER_SIZE - 1), seq);

			std::mutex waiting_mutex;
			std::condition_variable waiting_condition;
			bool is_waiting = false;
			bool is_done = false;
			int64_t expected_full_seq = decltype(this->sequencer_)::INITIAL_CURSOR_VALUE + this->sequencer_.get_buffer_size();
			ASSERT_EQ(expected_full_seq, this->sequencer_.get_cursor());

			std::thread t([this, &waiting_mutex, &waiting_condition, &is_waiting, &is_done]
				{
					{
						std::lock_guard<std::mutex> lock(waiting_mutex);
						is_waiting = true;
					}
					waiting_condition.notify_one();

					int64_t next = this->sequencer_.next();
					this->sequencer_.publish(next);

					{
						std::lock_guard<std::mutex> lock(waiting_mutex);
						is_done = true;
					}
					waiting_condition.notify_one();
				});
			{
				std::unique_lock<std::mutex> lock(waiting_mutex);
				waiting_condition.wait(lock, [&is_waiting] { return is_waiting; });
			}
			ASSERT_EQ(expected_full_seq, this->sequencer_.get_cursor());

			this->gating_sequence_.set(decltype(this->sequencer_)::INITIAL_CURSOR_VALUE + 1);

			{
				std::unique_lock<std::mutex> lock(waiting_mutex);
				waiting_condition.wait(lock, [&is_done] { return is_done; });
			}
			ASSERT_EQ(expected_full_seq + 1, this->sequencer_.get_cursor());

			t.join();
		}

		TYPED_TEST(sequencer_test, should_throw_insufficient_capacity_exception_when_sequencer_is_full)
		{
			this->sequencer_.add_gating_sequences(std::vector<sequence*> { &this->gating_sequence_ });
			for (int i = 0; i < this->BUFFER_SIZE; i++)
			{
				this->sequencer_.next();
			}
			ASSERT_THROW(this->sequencer_.try_next(), insufficient_capacity_exception);
		}

		TYPED_TEST(sequencer_test, should_calculate_remaining_capacity)
		{
			this->sequencer_.add_gating_sequences(std::vector<sequence*> { &this->gating_sequence_ });
			ASSERT_EQ((int64_t)this->BUFFER_SIZE, this->sequencer_.remaining_capacity());
			for (int i = 1; i < this->BUFFER_SIZE; i++)
			{
				this->sequencer_.next();
				ASSERT_EQ((int64_t)this->BUFFER_SIZE - i, this->sequencer_.remaining_capacity());
			}
		}

		TYPED_TEST(sequencer_test, should_not_be_available_until_published)
		{
			int64_t next = this->sequencer_.next(6);
			for (int i = 0; i <= 5; i++)
			{
				ASSERT_FALSE(this->sequencer_.is_available(i));
			}

			this->sequencer_.publish(next - (6 - 1), next);

			for (int i = 0; i <= 5; i++)
			{
				ASSERT_TRUE(this->sequencer_.is_available(i));
			}
			ASSERT_FALSE(this->sequencer_.is_available(6));
		}

		TYPED_TEST(sequencer_test, should_notify_wait_strategy_on_publish)
		{
			auto& wait_strategy = this->mock_sequencer_.get_wait_strategy();
			EXPECT_CALL(wait_strategy, signal_all_when_blocking()).Times(1);

			this->mock_sequencer_.publish(this->mock_sequencer_.next());
		}

		TYPED_TEST(sequencer_test, should_notify_wait_strategy_on_publish_batch)
		{
			auto& wait_strategy = this->mock_sequencer_.get_wait_strategy();
			EXPECT_CALL(wait_strategy, signal_all_when_blocking()).Times(1);

			int64_t next = this->mock_sequencer_.next(4);
			this->mock_sequencer_.publish(next - (4 - 1), next);
		}

		TYPED_TEST(sequencer_test, should_wait_on_publication)
		{
			auto barrier = this->sequencer_.new_barrier();

			int64_t next = this->sequencer_.next(10);
			int64_t lo = next - (10 - 1);
			int64_t mid = next - 5;

			for (int64_t i = lo; i < mid; i++)
			{
				this->sequencer_.publish(i);
			}
			ASSERT_EQ(mid - 1, barrier->wait_for(-1));

			for (int64_t i = mid; i <= next; i++)
			{
				this->sequencer_.publish(i);
			}
			ASSERT_EQ(next, barrier->wait_for(-1));
		}

		TYPED_TEST(sequencer_test, should_try_next)
		{
			this->sequencer_.add_gating_sequences(std::vector<sequence*> { &this->gating_sequence_ });
			for (int64_t i = 0; i < this->BUFFER_SIZE; i++)
			{
				this->sequencer_.publish(this->sequencer_.try_next());
			}
			ASSERT_THROW(this->sequencer_.try_next(), insufficient_capacity_exception);
		}

		TYPED_TEST(sequencer_test, should_claim_specific_sequence)
		{
			int64_t seq = 14;

			this->sequencer_.claim(seq);
			this->sequencer_.publish(seq);
			ASSERT_EQ(seq + 1, this->sequencer_.next());
		}

		TYPED_TEST(sequencer_test, should_not_allow_bulk_next_less_than_zero)
		{
			ASSERT_THROW(this->sequencer_.next(-1), std::invalid_argument);
		}

		TYPED_TEST(sequencer_test, should_not_allow_bulk_next_of_zero)
		{
			ASSERT_THROW(this->sequencer_.next(0), std::invalid_argument);
		}

		TYPED_TEST(sequencer_test, should_not_allow_bulk_try_next_less_than_zero)
		{
			ASSERT_THROW(this->sequencer_.try_next(-1), std::invalid_argument);
		}

		TYPED_TEST(sequencer_test, should_not_allow_bulk_try_next_of_zero)
		{
			ASSERT_THROW(this->sequencer_.try_next(0), std::invalid_argument);
		}
	}
}
