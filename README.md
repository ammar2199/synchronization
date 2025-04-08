## A Collection of Synchronization Primitives

The following synchronization primitives are included and are built atop C/C++ atomics and futex():
- Mutex
- Mutex with Priority-Inheritance
- Condition Variable
- Semaphore

The implementation sits within sync.c and sync.cpp and can be used in both C & C++ projects. A C++ wrapper-class around our mutex is implemented within
mutex.cpp/mutex.h. This class conforms to the [Mutex Named Requirement](https://en.cppreference.com/w/cpp/named_req/Mutex).

Some tests and benchmarks are included aswell.\
To build and run:\
```$ make CTest && ./bin/CTest```\
```$ make CppTest && ./bin/CppTest```


### Mutex-PI Test & Benchmark
A small, and slightly contrived test showing the power of a PI-mutex is implemented in CppTest

To start the test:\
```$ make CppTest && ./bin/CppTest```\
then from another terminal\
```$ ./runperfetto.sh ```

#### Overview
We create 3 threads running on a single core and set their priorities such that one is low, medium, and high.
The medium priority thread constantly loops. We let the low-priority thread grab a mutex, then have the high-priority thread contend for the same mutex.
The results are shown below comparing both a pi-mutex and normal-mutex.

Perfetto is used for visualization and to gather Linux FTrace scheduling events and overlay our own app-events aswell.
The key track-event to observe below is the "Low-Pri Critical Section"; which shows the time it takes
for the low-priority thread to complete its critical section when both a normal mutex and PI-mutex is used.

![perfetto visual](https://github.com/ammar2199/synchronization/blob/main/images/perfetto.png)

