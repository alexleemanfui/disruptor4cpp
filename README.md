# disruptor4cpp
C++ port of LMAX disruptor.

I try to implement it as closely as possible to the Java version, with C++ features in mind.

The library requires C++11 features. Currently, it has been tested in GCC 4.8.

## What's new?
2015-09-01:
The core features except DSL from Java version **3.3.2** have been ported.

## Getting Started
The library is header-only. Clone and copy the "include" folder. For example,
```
$ git clone https://github.com/alexleemanfui/disruptor4cpp.git
$ cd disruptor4cpp
$ mkdir /opt/disruptor4cpp/
$ cp -pr include/ /opt/disruptor4cpp/
```

To run the test,
```
$ git clone https://github.com/alexleemanfui/disruptor4cpp.git
$ cd disruptor4cpp
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./disruptor4cpp_test
```

To use it, include the below header file
```cpp
#include <disruptor4cpp/disruptor4cpp.h>
```

## Example
```cpp
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>

#include <disruptor4cpp/disruptor4cpp.h>

class int_handler : public disruptor4cpp::event_handler<int>
{
public:
	int_handler() = default;
	virtual ~int_handler() = default;
	virtual void on_start() { }
	virtual void on_shutdown() { }
	virtual void on_event(int& event, int64_t sequence, bool end_of_batch)
	{
		std::cout << "Received integer: " << event << std::endl;
	}
	virtual void on_timeout(int64_t sequence) { }
	virtual void on_event_exception(const std::exception& ex, int64_t sequence, int* event) { }
	virtual void on_start_exception(const std::exception& ex) { }
	virtual void on_shutdown_exception(const std::exception& ex) { }
};

int main(int argc, char* argv[])
{
	using namespace disruptor4cpp;

	// Create the ring buffer.
	ring_buffer<int, 1024, busy_spin_wait_strategy, producer_type::multi> ring_buffer;

	// Create and run the consumer on another thread.
	auto barrier = ring_buffer.new_barrier();
	int_handler handler;
	batch_event_processor<decltype(ring_buffer)> processor(ring_buffer, std::move(barrier), handler);
	std::thread processor_thread([&processor] { processor.run(); });
	
	// Publish some integers.
	for (int i = 0; i < 1000; i++)
	{
		int64_t seq = ring_buffer.next();
		ring_buffer[seq] = i;
		ring_buffer.publish(seq);
	}

	// Stop the consumer.
	std::this_thread::sleep_for(std::chrono::seconds(1));
	processor.halt();
	processor_thread.join();
	return 0;
}
```
