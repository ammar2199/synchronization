CC = gcc
CFLAGS = -Wall

CXX = g++
CXXFLAGS = -std=c++20

BINDIR = bin

TEST_OBJECTS := tests/testconditionvariable.o tests/testmutex.o tests/testpimutex.o tests/testpimutexperf.o tests/testsemaphore.o tests/testutils.o
TEST_HEADERS := tests/*.h

$(shell mkdir -p $(BINDIR))

CTest: sync.o CTest.o $(TEST_OBJECTS) 
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $^

CppTest: sync.o CppTest.o tests/testpimutexperf.cpp perfetto.o mutex.o
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/$@ $^

tests/%.o: tests/%.c tests/%.h
	$(CC) $(CFLAGS) -c $< -o $@

mutex.o: mutex.cpp mutex.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

perfetto.o: perfetto.cc perfetto.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

CppTest.o: CppTest.cpp PerfettoCategories.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf *.o tests/*.o $(BINDIR)/*
