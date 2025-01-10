#include "testutils.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// ((1B * sec1) + ns1) - ((1B * sec2) + ns2)
void PrintRuntime(const struct timespec* const start,
                  const struct timespec* const end, const char* str) {
  assert(start && end && "Can't PrintRuntime() - NULL");
  uint64_t secDiff = end->tv_sec - start->tv_sec;
  uint64_t nsDiff = end->tv_nsec - start->tv_nsec;
  uint64_t elapsedNs = secDiff * 1000000000 + nsDiff;
  double elapsedMs = elapsedNs / 1000000.0;
  printf("%s elapsed time: %f\n", str, elapsedMs);
}

inline void PrintTestName(const char* name) {
  printf("==== Start %s ====\n", name);
}
