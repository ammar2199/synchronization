#include "mutex.h"
#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>
#include <unistd.h>

// Perfetto
#include "perfetto.h"
#include <condition_variable>

#include "tests/testpimutexperf.h"
#include "PerfettoCategories.h"

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

//
// Small synchronization::Mutex test
//

static int global = 0;
synchronize::Mutex mutex;
static constexpr int INCREMENTS = 10000;
static constexpr int NUM_THREADS = 10;

void ThreadIncrement() {
  for (int i=0; i<INCREMENTS; i++) {
    std::lock_guard<synchronize::Mutex> guard(mutex); 
    global++;
  }
}

// 
// PERFETTO
//

// Grabbed from /perfetto/examples/sdk/example_system_wide.cc
class Observer : public perfetto::TrackEventSessionObserver {
 public:
  Observer() { perfetto::TrackEvent::AddSessionObserver(this); }
  ~Observer() override { perfetto::TrackEvent::RemoveSessionObserver(this); }

  void OnStart(const perfetto::DataSourceBase::StartArgs&) override {
    std::unique_lock<std::mutex> lock(mutex);
    cv.notify_one();
  }

  void WaitForTracingStart() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [] { return perfetto::TrackEvent::IsEnabled(); });
  }

  std::mutex mutex;
  std::condition_variable cv;
};

static void InitializePerfetto();

int main(int argc, char** argv) {
  std::cout << "Start CppTest" << std::endl;
  // Brief Mutex Test
  {
    std::vector<std::jthread> jthreads;
    for (int i=0; i<NUM_THREADS; i++) {
      jthreads.push_back(std::jthread(ThreadIncrement));
    }
  }

  std::cout << "global: " << global << std::endl;
  assert(global == INCREMENTS * NUM_THREADS && "global != INCREMENT * NUM_THREADS");

  uid_t uid = geteuid();  
  if (uid != 0) {
    std::cout << "Can't run Mutex PI Perf Test & Perfetto trace without running with root privileges\n";
    return 1;
  }
 
  // Perfetto 
  std::cout << "Initializing Perfetto" << std::endl;
  InitializePerfetto();
  Observer observer;
  std::cout << "Waiting for Perfetto Tracing to Start" << std::endl;
  observer.WaitForTracingStart();
  assert(TRACE_EVENT_CATEGORY_ENABLED("pi") && "pi perfetto trace category NOT enabled");
  MutexPIPerfTest();  
  return 0;
}

static void InitializePerfetto() {
  perfetto::TracingInitArgs args;
  args.backends |= perfetto::kSystemBackend; 
  args.enable_system_consumer = false;
  perfetto::Tracing::Initialize(args);
  if (!perfetto::TrackEvent::Register()) {
    std::cerr << "Failed to Register Perfetto Categories" << std::endl;
    exit(1);
  }
}
