#include "testsemaphore.h"
#include "testutils.h"
#include "../sync.h"

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

static const int NUM_THREADS = 15;
static const int ENTRIES = 10000;
static const int MAX_THREADS_IN_CS = 5; 
semaphore_t semaphore;
atomic_int criticalSectionCount;

static void* ThreadWork(void*) {
  for (int i=0; i<ENTRIES; i++) {
    SemaphoreAcquire(&semaphore);
    assert(atomic_fetch_add_explicit(&criticalSectionCount, 1, memory_order_relaxed) < MAX_THREADS_IN_CS && "Too many threads in CS");
    for (int j=0; j<1000000; j++); // spin
    atomic_fetch_sub_explicit(&criticalSectionCount, 1, memory_order_relaxed);
    SemaphoreRelease(&semaphore);
  }
  return NULL;
}

bool SemaphoreTest() {
  PrintTestName("Semaphore Test");
  SemaphoreInit(&semaphore, MAX_THREADS_IN_CS);
  pthread_t threads[NUM_THREADS];
  for (int i=0; i<NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, ThreadWork, NULL);
  }
  for (int i=0; i<NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
  return true;
}

