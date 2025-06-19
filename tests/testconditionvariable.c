#include "testconditionvariable.h"
#include "testutils.h"
#include "../sync.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

static const int NUM_ROUNDS = 10000;

static condition_variable_t condVar;
static mutex_t mutex;
static bool condition = false;

static void* WaitingThread(void*) {
  MutexLock(&mutex);
  while (!condition) {
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
    pthread_create(&waiter, NULL, WaitingThread, NULL);
    pthread_create(&waiter2, NULL, WaitingThread, NULL);
    pthread_create(&notifier, NULL, NotifyingThread, NULL);
    pthread_join(waiter, NULL);
    pthread_join(waiter2, NULL);
    pthread_join(notifier, NULL);
    // XXX: Change to timedjoin -> and fail then fail test.
    condition = false;
  }
  return true;
}

static void* Ping(void*) {
  for (int i=0; i<10; i++) {
    MutexLock(&mutex);
    while (!condition) ConditionVariableWait(&condVar, &mutex); // Wait until condition is true
    condition = false; // Reset to false
    ConditionVariableNotifyOne(&condVar);
    MutexUnlock(&mutex);
  }
  
  return NULL;
}

static void* Pong(void*) {
  for (int i=0; i<10; i++) {
    MutexLock(&mutex);
    condition = true;
    ConditionVariableNotifyOne(&condVar);
    while (condition) ConditionVariableWait(&condVar, &mutex); // Wait until condition is false
    MutexUnlock(&mutex);
  }
  return NULL;
}

bool ConditionVariableTest2() {
  PrintTestName("Condition Variable Test2 - PingPong Test");
  condition = false;
  for (int i=0; i<NUM_ROUNDS; i++) {
    MutexInit(&mutex, 0);
    ConditionVariableInit(&condVar);
    pthread_t a, b;
    pthread_create(&a, NULL, Ping, NULL);
    pthread_create(&b, NULL, Pong, NULL);
    pthread_join(a, NULL); 
    pthread_join(b, NULL); 
  }
  return true;
}

