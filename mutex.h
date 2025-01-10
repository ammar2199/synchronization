#pragma once 
#include "sync.h"

namespace synchronize {

// Should conform to Mutex Named Requirement
class Mutex {
  public:
    Mutex(bool priorityInheritance = false);
    Mutex(const Mutex& m) = delete; 
    Mutex(Mutex&& m) = delete;
    Mutex& operator=(const Mutex& m) = delete;
    void lock();
    void unlock();
    bool try_lock();
  private:
    mutex_t mLock;
};

}; // namespace synchronize

