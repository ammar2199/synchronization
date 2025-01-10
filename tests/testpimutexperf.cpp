#define _GNU_SOURCE
#include "testpimutexperf.h"
#include "../sync.h"

#include <sched.h>
#include <linux/sched.h>

#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

// PERFETTO
#include "../perfetto.h"
#include "../PerfettoCategories.h"

// ((1B * sec1) + ns1) - ((1B * sec2) + ns2)
static void PrintRuntimeCpp(const struct timespec* const start,
                  const struct timespec* const end, const char* str) {
  assert(start && end && "Can't PrintRuntime() - NULL");
  uint64_t secDiff = end->tv_sec - start->tv_sec;
  uint64_t nsDiff = end->tv_nsec - start->tv_nsec;
  uint64_t elapsedNs = secDiff * 1000000000 + nsDiff;
  double elapsedMs = elapsedNs / 1000000.0;
  printf("%s elapsed time: %f\n", str, elapsedMs);
}

static pthread_cond_t priorityCv;
static pthread_mutex_t conditionLock;
static bool LowPriorityHasLock = false;
static struct mutex_t piMutex;
static struct mutex_t noPiMutex;

static bool RunOnCore(int c) {
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  CPU_SET(c, &affinity);
  int err = sched_setaffinity(0, 1, &affinity);
  if (err) {
    perror("Failed to set affinity mask");
    return false;
  }
  return true;
}

static bool SetSchedPolicyAndPriority(int policy, int priority) {
  struct sched_param param = {
    .sched_priority=priority
  };
  if (sched_setscheduler(0, policy, &param)) {
    perror("Failed to set priority");
    return false;
  }
  return true;
}
 
static void* HighPri(void* arg) {
  mutex_t* const m = (mutex_t*)(arg); 

  RunOnCore(0);
  SetSchedPolicyAndPriority(SCHED_FIFO, 10);
    
  // Wait until LowPri grabs lock 
  pthread_mutex_lock(&conditionLock);
  while (LowPriorityHasLock == false) {
    pthread_cond_wait(&priorityCv, &conditionLock);
  }
  pthread_mutex_unlock(&conditionLock);
  TRACE_EVENT_BEGIN("pi", "High-Pri Lock Acquire");
  MutexLock(m); // contend for lock
  TRACE_EVENT_END("pi");
  for (int i=0; i<1000000; i++); // spin
  MutexUnlock(m);
  return NULL;
}

static void* MedPri(void*) {
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  RunOnCore(0);
  SetSchedPolicyAndPriority(SCHED_FIFO, 5);
  while(true); // Spin  
  return NULL;
}

static void* LowPri(void* arg) {
  mutex_t* const m = (mutex_t*)(arg); 

  RunOnCore(0);
  // Contend for lock
  MutexLock(m);
  TRACE_EVENT_BEGIN("pi", "Low-Pri Critical Section");
  pthread_mutex_lock(&conditionLock);
  LowPriorityHasLock = true;
  pthread_mutex_unlock(&conditionLock); 
  pthread_cond_signal(&priorityCv);
  for (uint64_t i=0; i<1000000000; i++); // Spin
  TRACE_EVENT_END("pi");
  MutexUnlock(m);
  return NULL;
}

// TODO: Change priority through pthread.
void MutexPIPerfTest() {
  pthread_cond_init(&priorityCv, NULL);
  pthread_mutex_init(&conditionLock, NULL);
  
  MutexInit(&piMutex, MUTEX_PRIORITY_INHERITANCE);
  MutexInit(&noPiMutex, 0);
  
  // Timing
  struct timespec start, end;
  // Threads
  pthread_t highPriThread, mediumPriThread, lowPriThread;
  
  // Normal Mutex
  TRACE_EVENT_BEGIN("pi", "Normal Mutex");
  clock_gettime(CLOCK_REALTIME, &start);
  pthread_create(&mediumPriThread, NULL, MedPri, NULL);
  pthread_create(&highPriThread, NULL, HighPri, &noPiMutex);
  pthread_create(&lowPriThread, NULL, LowPri, &noPiMutex);

  pthread_join(lowPriThread, NULL);
  pthread_join(highPriThread, NULL);
  clock_gettime(CLOCK_REALTIME, &end);
  TRACE_EVENT_END("pi");
  PrintRuntimeCpp(&start, &end, "Normal Mutex");

  pthread_cancel(mediumPriThread);

  // PI-Mutex
  LowPriorityHasLock = false;

  TRACE_EVENT_BEGIN("pi", "PI Mutex");
  clock_gettime(CLOCK_REALTIME, &start);

  pthread_create(&mediumPriThread, NULL, MedPri, NULL);
  pthread_create(&highPriThread, NULL, HighPri, &piMutex);
  pthread_create(&lowPriThread, NULL, LowPri, &piMutex);

  pthread_join(lowPriThread, NULL);
  pthread_join(highPriThread, NULL);
  
  clock_gettime(CLOCK_REALTIME, &end);
  TRACE_EVENT_END("pi");
  PrintRuntimeCpp(&start, &end, "PI Mutex");

  pthread_cancel(mediumPriThread);
  
  pthread_cond_destroy(&priorityCv);
  pthread_mutex_destroy(&conditionLock);
}

