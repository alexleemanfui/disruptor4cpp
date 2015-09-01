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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <disruptor4cpp/disruptor4cpp.h>
#include "support/stub_event.h"
#include "utils/count_down_latch.h"

namespace disruptor4cpp
{
	namespace test
	{
		class stub_event_processor
		{
		public:
			stub_event_processor() = default;
			~stub_event_processor() = default;

			void set_sequence(int64_t value)
			{
				sequence_.set(value);
			}

			sequence& get_sequence()
			{
				return sequence_;
			}

			void halt()
			{
				running_.store(false, std::memory_order_release);
			}

			bool is_running() const
			{
				return running_.load(std::memory_order_acquire);
			}

			void run()
			{
				bool expected_running_state = false;
				if (!running_.compare_exchange_strong(expected_running_state, true))
					throw std::runtime_error("Thread is already running");
			}

		private:
			sequence sequence_;
			std::atomic<bool> running_;
		};

		class count_down_latch_sequence
		{
		public:
			count_down_latch_sequence()
				: sequence_(-1),
				  default_latch_(0),
				  latch_(default_latch_)
			{
			}

			count_down_latch_sequence(int64_t initial_value, count_down_latch& latch)
				: sequence_(initial_value),
				  default_latch_(0),
				  latch_(latch)
			{
			}

			~count_down_latch_sequence() = default;

			int64_t get() const
			{
				latch_.count_down();
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
			count_down_latch_sequence(const count_down_latch_sequence&) = delete;
			count_down_latch_sequence& operator=(const count_down_latch_sequence&) = delete;
			count_down_latch_sequence(count_down_latch_sequence&&) = delete;
			count_down_latch_sequence& operator=(count_down_latch_sequence&&) = delete;

			std::atomic<int64_t> sequence_;
			count_down_latch default_latch_;
			count_down_latch& latch_;
		};

		class mock_event_processor
		{
		public:
			MOCK_CONST_METHOD0(get_sequence, sequence&());
		};

		class mock_event_processor_latch
		{
		public:
			MOCK_CONST_METHOD0(get_sequence, count_down_latch_sequence&());
		};

		class sequencer_barrier_test : public testing::Test
		{
		protected:
			void SetUp() override
			{
				auto seq_barrier = ring_buffer_.new_barrier();
				no_op_event_processor_.reset(
					new no_op_event_processor<decltype(ring_buffer_)>(ring_buffer_, std::move(seq_barrier)));
				ring_buffer_.add_gating_sequences(std::vector<sequence*> { &no_op_event_processor_->get_sequence() });
			}

			template <typename TRingBuffer>
			void fill_ring_buffer(TRingBuffer& ring_buffer, int64_t expected_number_messages)
			{
				for (int64_t i = 0; i < expected_number_messages; i++)
				{
					int64_t seq = ring_buffer.next();
					auto& event = ring_buffer[seq];
					event.set_value((int)i);
					ring_buffer.publish(seq);
				}
			}

			static constexpr int BUFFER_SIZE = 64;

			ring_buffer<stub_event, BUFFER_SIZE, blocking_wait_strategy, producer_type::multi> ring_buffer_;
			testing::NiceMock<mock_event_processor> event_processor1_;
			testing::NiceMock<mock_event_processor> event_processor2_;
			testing::NiceMock<mock_event_processor> event_processor3_;
			std::unique_ptr<no_op_event_processor<decltype(ring_buffer_)>> no_op_event_processor_;
		};

		TEST_F(sequencer_barrier_test, should_wait_for_work_complete_where_complete_work_threshold_is_ahead)
		{
			int64_t expected_number_messages = 10;
			int64_t expected_work_sequence = 9;
			fill_ring_buffer(this->ring_buffer_, expected_number_messages);

			sequence sequence1(expected_number_messages);
			sequence sequence2(expected_work_sequence);
			sequence sequence3(expected_number_messages);

			EXPECT_CALL(event_processor1_, get_sequence())
				.WillOnce(testing::ReturnRef(sequence1));
			EXPECT_CALL(event_processor2_, get_sequence())
				.WillOnce(testing::ReturnRef(sequence2));
			EXPECT_CALL(event_processor3_, get_sequence())
				.WillOnce(testing::ReturnRef(sequence3));

			auto seq_barrier = ring_buffer_.new_barrier(std::vector<sequence*>
			{
				&event_processor1_.get_sequence(),
				&event_processor2_.get_sequence(),
				&event_processor3_.get_sequence()
			});
			int64_t completed_work_sequence = seq_barrier->wait_for(expected_work_sequence);
			ASSERT_GE(completed_work_sequence, expected_work_sequence);
		}

		TEST_F(sequencer_barrier_test, should_wait_for_work_complete_where_all_workers_are_blocked_on_ring_buffer)
		{
			int64_t expected_number_messages = 10;
			fill_ring_buffer(this->ring_buffer_, expected_number_messages);

			std::array<stub_event_processor, 3> workers;
			std::vector<sequence*> sequences_to_track;
			for (auto& stub_worker : workers)
			{
				stub_worker.set_sequence(expected_number_messages - 1);
				sequences_to_track.push_back(&stub_worker.get_sequence());
			}
			auto seq_barrier = ring_buffer_.new_barrier(sequences_to_track);

			std::thread t([this, &workers]()
			{
				int64_t seq = ring_buffer_.next();
				auto& event = ring_buffer_[seq];
				event.set_value((int)seq);
				ring_buffer_.publish(seq);
				for (auto& stub_worker : workers)
				{
					stub_worker.set_sequence(seq);
				}
			});

			int64_t expected_work_sequence = expected_number_messages;
			int64_t completed_work_sequence = seq_barrier->wait_for(expected_number_messages);
			ASSERT_GE(completed_work_sequence, expected_work_sequence);
			t.join();
		}

		TEST_F(sequencer_barrier_test, should_interrupt_during_busy_spin)
		{
			ring_buffer<stub_event, BUFFER_SIZE, blocking_wait_strategy, producer_type::multi,
				count_down_latch_sequence> ring_buffer;

			testing::NiceMock<mock_event_processor_latch> event_processor1;
			testing::NiceMock<mock_event_processor_latch> event_processor2;
			testing::NiceMock<mock_event_processor_latch> event_processor3;

			int64_t expected_number_messages = 10;
			fill_ring_buffer(ring_buffer, expected_number_messages);

			count_down_latch latch(3);
			count_down_latch_sequence sequence1(8, latch);
			count_down_latch_sequence sequence2(8, latch);
			count_down_latch_sequence sequence3(8, latch);

			EXPECT_CALL(event_processor1, get_sequence())
				.WillOnce(testing::ReturnRef(sequence1));
			EXPECT_CALL(event_processor2, get_sequence())
				.WillOnce(testing::ReturnRef(sequence2));
			EXPECT_CALL(event_processor3, get_sequence())
				.WillOnce(testing::ReturnRef(sequence3));

			auto seq_barrier = ring_buffer.new_barrier(std::vector<count_down_latch_sequence*>
			{
				&event_processor1.get_sequence(),
				&event_processor2.get_sequence(),
				&event_processor3.get_sequence()
			});

			bool alerted = false;
			std::thread t([&expected_number_messages, &seq_barrier, &alerted]()
			{
				try
				{
					seq_barrier->wait_for(expected_number_messages - 1);
				}
				catch (alert_exception&)
				{
					alerted = true;
				}
				catch (...)
				{
				}
			});

			latch.wait(std::chrono::seconds(3));
			seq_barrier->alert();
			t.join();

			ASSERT_TRUE(alerted);
		}

		TEST_F(sequencer_barrier_test, should_wait_for_work_complete_where_complete_work_threshold_is_behind)
		{
			int64_t expected_number_messages = 10;
			fill_ring_buffer(this->ring_buffer_, expected_number_messages);

			std::array<stub_event_processor, 3> workers;
			std::vector<sequence*> sequences_to_track;
			for (auto& stub_worker : workers)
			{
				stub_worker.set_sequence(expected_number_messages - 2);
				sequences_to_track.push_back(&stub_worker.get_sequence());
			}
			auto seq_barrier = ring_buffer_.new_barrier(sequences_to_track);

			std::thread t([this, &workers]()
			{
				for (auto& stub_worker : workers)
				{
					stub_worker.set_sequence(stub_worker.get_sequence().get() + 1);
				}
			});
			t.join();

			int64_t expected_work_sequence = expected_number_messages - 1;
			int64_t completed_work_sequence = seq_barrier->wait_for(expected_work_sequence);
			ASSERT_GE(completed_work_sequence, expected_work_sequence);
		}

		TEST_F(sequencer_barrier_test, should_set_and_clear_alert_status)
		{
			auto seq_barrier = ring_buffer_.new_barrier();
			ASSERT_FALSE(seq_barrier->is_alerted());

			seq_barrier->alert();
			ASSERT_TRUE(seq_barrier->is_alerted());

			seq_barrier->clear_alert();
			ASSERT_FALSE(seq_barrier->is_alerted());
		}
	}
}
