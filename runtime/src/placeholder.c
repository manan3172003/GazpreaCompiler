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


void throwArraySizeError() {
  SizeError("Array size cannot be smaller than the actual array size");
}
void throwVectorSizeError() {
    SizeError("Vector size mismatch");
}

enum ElementType {
  ELEM_INT = 0,
  ELEM_REAL = 1,
  ELEM_CHAR = 2,
  ELEM_BOOL = 3,
  ELEM_ARRAY = 4
};

typedef struct {
  int32_t size;
  void* data;
  int8_t is2D;  // boolean
} ArrayStruct;

void printArray(ArrayStruct* arrayStruct, int32_t elementType) {
  printf("[");

  for (int32_t i = 0; i < arrayStruct->size; i++) {
    if (i > 0) {
      printf(" ");
    }

    if (arrayStruct->is2D) {
      ArrayStruct* innerArrays = (ArrayStruct*)arrayStruct->data;
      printArray(&innerArrays[i], elementType);
    } else {
      switch (elementType) {
        case ELEM_INT: {
          int32_t* intData = (int32_t*)arrayStruct->data;
          printf("%d", intData[i]);
          break;
        }
        case ELEM_REAL: {
          float* floatData = (float*)arrayStruct->data;
          printf("%g", (double)floatData[i]);
          break;
        }
        case ELEM_CHAR: {
          int8_t* charData = (int8_t*)arrayStruct->data;
          printf("%c", charData[i]);
          break;
        }
        case ELEM_BOOL: {
          int8_t* boolData = (int8_t*)arrayStruct->data;
          printf("%c", boolData[i] ? 'T' : 'F');
          break;
        }
      }
    }
  }

  printf("]");
}
