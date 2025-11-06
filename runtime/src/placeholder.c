#include <stdio.h>
#include <stdint.h>
#include "run_time_errors.h"

// Add function named dummyPrint with signature void(int) to llvm to have this linked in.
void dummyPrint(int i) {
  printf("I'm a function! %d\n", i);
}

int32_t ipow(int32_t base, int32_t exp) {
  int32_t result = 1;
  for (int32_t i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

void throwDivisionByZeroError() {
    MathError("Division by zero");
}