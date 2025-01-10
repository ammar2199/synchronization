#include "testconditionvariable.h"
#include "testutils.h"
#include "../sync.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

static const int NUM_ROUNDS = 100000;

static condition_variable_t condVar;
static mutex_t mutex;
static bool condition = false;

static void* WaitingThread(void*) {
  MutexLock(&mutex);
  while (condition == false) {
    ConditionVariableWait(&condVar, &mutex);
  }
  assert(condition == true && "Awoken Thread sees false condition");
  MutexUnlock(&mutex);
  return NULL;
}

static void* NotifyingThread(void*) {
  MutexLock(&mutex);
  condition = true;
  MutexUnlock(&mutex);
  ConditionVariableNotifyOne(&condVar);
  ConditionVariableNotifyOne(&condVar);
  // ConditionVariableNotifyAll(&condVar);
  return NULL;
}

bool ConditionVariableTest() {
  PrintTestName("Condition Variable Test");
  for (int i=0; i<NUM_ROUNDS; i++) {
    MutexInit(&mutex, 0);
    ConditionVariableInit(&condVar);
    pthread_t waiter;
    pthread_t waiter2;
    pthread_t notifier;
    pthread_create(&waiter, NULL, WaitingThread, 0);
    pthread_create(&waiter2, NULL, WaitingThread, 0);
    pthread_create(&notifier, NULL, NotifyingThread, 0);
    pthread_join(waiter, NULL);
    pthread_join(waiter2, NULL);
    pthread_join(notifier, NULL);
    // XXX: Change to timedjoin -> and fail then fail test.
    condition = false;
  }
  return true;
}
