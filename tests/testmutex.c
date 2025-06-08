#include "testmutex.h"
#include "testutils.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "../sync.h"

#include <time.h>

static uint64_t global = 0;
static struct mutex_t mutex;
static const int NUM_INCREMENTS = 10000;
static const int NUM_THREADS = 10;
static const int NUM_ROUNDS = 100;

static void* MutexTestWorker(void*) {
  for (int i=0; i<NUM_INCREMENTS; i++) {
    int err = MutexLock(&mutex);
    assert(err == 0);
    global++;
    err = MutexUnlock(&mutex);
    assert(err == 0);
  }
  return NULL; 
}

bool MutexTest() {
  PrintTestName("Mutex Test");
  pthread_t threads[NUM_THREADS]; 
  uint32_t flags[2] = {MUTEX_ALWAYS_WAKE, 0};
  const char* strs[2] = {"Always-Wake Mutex", "Normal Mutex"};
  for (int f=0; f<2; f++) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i=0; i<NUM_ROUNDS; i++) {
      global = 0;
      MutexInit(&mutex, flags[f]);
      for (int t=0; t<NUM_THREADS; t++) {
        pthread_create(&threads[t], NULL, MutexTestWorker, NULL);
      }
      
      for (int t=0; t<NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
      }
      //assert(global == NUM_THREADS * NUM_INCREMENTS && "Failed Mutex Test Run");
      if (global != NUM_THREADS * NUM_INCREMENTS) {
        printf("Mutex Test Run %d Failed! global=%ld\n", i, global);
        return false;
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    PrintRuntime(&start, &end, strs[f]);
  }
  return true;
}
