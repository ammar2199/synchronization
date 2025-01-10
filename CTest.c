#include "sync.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "tests/testmutex.h"
#include "tests/testpimutex.h"
#include "tests/testpimutexperf.h"
#include "tests/testsemaphore.h"
#include "tests/testconditionvariable.h"

#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define WHITE "\x1b[37m"


#define LOG_RESULT(x)       \
  do {                      \
    if (x) printf(GREEN "Passed!\n" WHITE); \
    else printf(RED "Failed!\n" WHITE); \
  } while(0)                  \

int main(int argc, char** argv) {
  printf("Start Tests\n");
  uid_t uid = geteuid();

  LOG_RESULT(MutexTest());
 
  LOG_RESULT(MutexPITest());
  
  if (uid == 0) MutexPIPerfTest();
  else printf("Can't run Mutex PI Perf Test without root privileges\n");

  LOG_RESULT(SemaphoreTest());

  LOG_RESULT(ConditionVariableTest());
   
  printf("End Tests\n");
  return 0;
} 
