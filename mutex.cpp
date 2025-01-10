#include "mutex.h"
#include "sync.h"
#include <system_error>

namespace synchronize {

Mutex::Mutex(bool priorityInheritance) {
  uint32_t flags = 0;
  flags |= priorityInheritance ? MUTEX_PRIORITY_INHERITANCE : 0;
  MutexInit(&mLock, flags);
}

void Mutex::lock() {
  int err = MutexLock(&mLock);
  if (err) throw std::system_error(err, std::system_category(), "failed to lock mutex");
}

void Mutex::unlock() {
  MutexUnlock(&mLock);
}

bool Mutex::try_lock() {
  return MutexTryLock(&mLock) == 0;
}

}; // namespace synchronize
