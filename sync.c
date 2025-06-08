#define _GNU_SOURCE

#include "sync.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>

// TODO: Test on arm64 with different memory_orders

static inline long futex(uint32_t* addr, int futex_op, uint32_t val) {
  return syscall(SYS_futex, addr, futex_op, val, NULL, NULL, NULL);
}

int MutexInit(mutex_t* const m, uint32_t flags) {
  if (!m || flags > 3) return EINVAL;
  m->mLock = 0;
  m->mFlags = flags;
  return 0;
}

static int MutexLockNormal(mutex_t* const m) {
  int expected = 0;
  // Attempt to acquire lock.
  if (!atomic_compare_exchange_strong_explicit(&m->mLock, &expected, 1, memory_order_acquire, memory_order_relaxed)) {

    do {
      // Blocks. Note: futex / FUTEX_WAIT, loads and compares the futex-word atomically.
      long err = syscall(SYS_futex, (uint32_t*)&m->mLock, FUTEX_WAIT, 1, NULL, NULL, NULL);
      // Either we were woken up, never slept, or a spurious wake-up occured. 
      if (err == EINTR) return EINTR; // Woken up by signal. Report
      expected = 0; 
    } while (!atomic_compare_exchange_strong_explicit(&m->mLock, &expected, 1, memory_order_acquire, memory_order_relaxed));
  }
  // Lock Acquired
  return 0;
}

static int MutexLockNormal2(mutex_t* const m) {
  int expected = 0;
  // Try to acquire lock 
  if (!atomic_compare_exchange_strong_explicit(&m->mLock, &expected, 1, memory_order_acquire, memory_order_relaxed)) {
    do {
      // Failed to acquire lock - need to sleep, set mLock = 2, so unlocking-thread knows to wake.
      expected = 1;
      // Signal that we're a waiting thread. This will either fail or succeed.
      atomic_compare_exchange_strong_explicit(&m->mLock, &expected, 2, memory_order_relaxed, memory_order_relaxed);
      long err = futex((uint32_t*)&m->mLock, FUTEX_WAIT, 2); // If the operation above was successful
                                                             // or the operation failed but m->mLock was already 2 we will
                                                             // FUTEX_WAIT. If it wasn't, let's try to acquire the lock again.
      // Either we were woken up, never slept, or a spurious wake-up occured.
      if (err == EINTR) return EINTR; // Woken up by signal. Report.
      expected = 0;
    } while (!atomic_compare_exchange_strong_explicit(&m->mLock, &expected, 2, memory_order_acquire, memory_order_relaxed));
  }
  return 0;
}

// XXX: No spurious-wakeups seen?
static int MutexLockPi(mutex_t* const m) {
  pid_t tid = gettid();
  int expected = 0;
  if (!atomic_compare_exchange_strong_explicit(&m->mLock, &expected, (int)tid, memory_order_acquire, memory_order_acquire)) {
    long err = syscall(SYS_futex, (uint32_t*)&m->mLock, FUTEX_LOCK_PI, 0, NULL, NULL, NULL);
    return err;
  }
  // Acquired lock...
  return 0;
}

int MutexLock(mutex_t* const m) {
  if (!m) return EINVAL;
  if (m->mFlags & MUTEX_PRIORITY_INHERITANCE) return MutexLockPi(m);
  if (m->mFlags & MUTEX_ALWAYS_WAKE) return MutexLockNormal(m);
  return MutexLockNormal2(m);
}

static int MutexUnlockNormal(mutex_t* const m) { 
  atomic_store_explicit(&m->mLock, 0, memory_order_release);
  syscall(SYS_futex, (uint32_t*) &m->mLock, 1, NULL, NULL, NULL);
  return 0;
}

static int MutexUnlockNormal2(mutex_t* const m) {
  if (atomic_exchange_explicit(&m->mLock, 0, memory_order_release) == 2) {
    futex((uint32_t*)&m->mLock, FUTEX_WAKE, 1);
  }
  return 0;
}

static int MutexUnlockPi(mutex_t* const m) {
  pid_t tid = gettid();
  if (!atomic_compare_exchange_strong_explicit(&m->mLock, &tid, 0, memory_order_release, memory_order_relaxed)) {
    long err = syscall(SYS_futex, (uint32_t*)&m->mLock, FUTEX_UNLOCK_PI, 0, NULL, NULL, NULL);
    return err;
  }
  return 0;
}

int MutexUnlock(mutex_t* const m) {
  if (!m) return EINVAL;
  if (m->mFlags & MUTEX_PRIORITY_INHERITANCE) return MutexUnlockPi(m);
  if (m->mFlags & MUTEX_ALWAYS_WAKE) return MutexUnlockNormal(m);
  return MutexUnlockNormal2(m);
}

static int MutexTryLockNormal(mutex_t* const m) {
 int expected = 0;
 if (atomic_compare_exchange_strong_explicit(&m->mLock, &expected, 1, memory_order_acquire, memory_order_relaxed)) {
   return 0;
 }
 return EBUSY;
}

static int MutexTryLockPi(mutex_t* const m) {
  int expected = 0;
  pid_t tid = gettid();
  if (atomic_compare_exchange_strong_explicit(&m->mLock, &expected, (int)tid, memory_order_acquire, memory_order_relaxed)) {
    return 0;
  }
  return EBUSY;
}

int MutexTryLock(mutex_t* const m) {
  if (!m) return EINVAL;
  if (m->mFlags & MUTEX_PRIORITY_INHERITANCE) return MutexTryLockPi(m);   
  return MutexTryLockNormal(m);
}

int SemaphoreInit(semaphore_t* const sem, unsigned int initialCount) {
  if (!sem) return EINVAL;
  sem->mCounter = initialCount;
  return 0;
}

int SemaphoreAcquire(semaphore_t* const sem) {
  if (!sem) return EINVAL;
  int count;
  do {
    count = atomic_load(&sem->mCounter);
    while (count == 0) {
      // Count is zero. Block until woken up by a releaser
      // Note: futex loads and compares futex-word atomically.
      long err = syscall(SYS_futex, &sem->mCounter, FUTEX_WAIT, 0, NULL, NULL, NULL);
      if (err == EINTR) return EINTR; // Awoken by signal. Return.
      count = atomic_load(&sem->mCounter); // Reload count
    }
  } while (!atomic_compare_exchange_strong(&sem->mCounter, &count, count - 1)); // Attempt to acquire resource
  return 0;
}

int SemaphoreRelease(semaphore_t* const sem) {
  if (!sem) return EINVAL;
  atomic_fetch_add(&sem->mCounter, 1);
  syscall(SYS_futex, &sem->mCounter, FUTEX_WAKE, 1, NULL, NULL, NULL);
  return 0;
}

int SemaphoreP(semaphore_t* const sem) {
  return SemaphoreAcquire(sem);
}

int SemaphoreV(semaphore_t* const sem) {
  return SemaphoreRelease(sem);
}

int ConditionVariableInit(condition_variable_t* const cv) {
  if (!cv) return EINVAL;
  cv->mConditionChecked = 0;
  return 0;
}

int ConditionVariableWait(condition_variable_t* const cv, mutex_t* const m) {
  if (!cv || !m) return EINVAL;
  atomic_store(&cv->mConditionChecked, 1); // Signal that we've checked condition.
  MutexUnlock(m);
  // Block until woken up
  long err = syscall(SYS_futex, &cv->mConditionChecked, FUTEX_WAIT, 1, NULL, NULL, NULL);
  if (err == EINTR) return EINTR;
  MutexLock(m); // Re-acquire the lock, then go check condition again.
  return 0; 
}

int ConditionVariableNotifyOne(condition_variable_t* const cv) {
  atomic_store(&cv->mConditionChecked, 0); // Waiter needs to recheck-condition
  syscall(SYS_futex, &cv->mConditionChecked, FUTEX_WAKE, 1, NULL, NULL, NULL);
  return 0;
}

int ConditionVariableNotifyAll(condition_variable_t* const cv) {
  atomic_store(&cv->mConditionChecked, 0); // Waiter needs to recheck-condition
  syscall(SYS_futex, &cv->mConditionChecked, FUTEX_WAKE, INT_MAX, NULL, NULL, NULL);
  return 0;
}

