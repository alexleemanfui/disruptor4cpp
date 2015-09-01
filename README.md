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
