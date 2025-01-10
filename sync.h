#pragma once
#ifdef __cplusplus
#include <cstdint>
#include <atomic>

#define NS(x) std::x

extern "C"
{

#else 
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>

#define NS(x) x
#endif 

#define MUTEX_PRIORITY_INHERITANCE 0x1

// I want a library that's compatible with C and C++
// Hence, write it in C, and let it be used in C++ too.
typedef struct mutex_t {
  NS(atomic_int) mLock;
  bool mPiEnabled;
} mutex_t; 

int MutexInit(mutex_t* const m, uint32_t flags);
int MutexLock(mutex_t* const m);
int MutexUnlock(mutex_t* const m);
int MutexTryLock(mutex_t* const m);

typedef struct semaphore_t {
  NS(atomic_int) mCounter;
} semaphore_t;

int SemaphoreInit(semaphore_t* const sem, unsigned int initialCount);
int SemaphoreAcquire(semaphore_t* const sem);
int SemaphoreRelease(semaphore_t* const sem);
int SemaphoreP(semaphore_t* const sem);
int SemaphoreV(semaphore_t* const sem);

typedef struct condition_variable_t {
  NS(atomic_int) mConditionChecked;
} condition_variable_t;

int ConditionVariableInit(condition_variable_t* const cv);
int ConditionVariableWait(condition_variable_t* const cv, mutex_t* const m);
int ConditionVariableNotifyOne(condition_variable_t* const cv);
int ConditionVariableNotifyAll(condition_variable_t* const cv);

#ifdef __cplusplus
}
#endif 
