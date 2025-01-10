#pragma once 

#include <time.h>

// ((1B * sec1) + ns1) - ((1B * sec2) + ns2)
void PrintRuntime(const struct timespec* const start,
                  const struct timespec* const end,
                  const char* str);

void PrintTestName(const char* name);

