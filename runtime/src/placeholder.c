#include "run_time_errors.h"
#include <stdint.h>
#include <stdio.h>

int32_t ipow_019addc8_6352_7de5_8629_b0688522175f(int32_t base, int32_t exp) {
  int32_t result = 1;
  for (int32_t i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

void throwDivisionByZeroError_019addc8_a29b_740a_9b09_8a712296bc1a() {
  MathError("Division by zero");
}

void throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c() {
  SizeError("Array size cannot be smaller than the actual array size");
}
void throwVectorSizeError_019addc9_1a57_7674_b3dd_79d0624d2029() {
  SizeError("Vector size mismatch");
}
void throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e() {
  IndexError("Array index out of range");
}

enum ElementType { ELEM_INT = 0, ELEM_REAL = 1, ELEM_CHAR = 2, ELEM_BOOL = 3, ELEM_ARRAY = 4 };

typedef struct {
  int32_t size;
  void *data;
  int8_t is2D; // boolean
} ArrayStruct;

void printArray_019addab_1674_72d4_aa4a_ac782e511e7a(ArrayStruct *arrayStruct,
                                                     int32_t elementType) {
  printf("[");

  for (int32_t i = 0; i < arrayStruct->size; i++) {
    if (i > 0) {
      printf(" ");
    }

    if (arrayStruct->is2D) {
      ArrayStruct *innerArrays = (ArrayStruct *)arrayStruct->data;
      printArray_019addab_1674_72d4_aa4a_ac782e511e7a(&innerArrays[i], elementType);
    } else {
      switch (elementType) {
      case ELEM_INT: {
        int32_t *intData = (int32_t *)arrayStruct->data;
        printf("%d", intData[i]);
        break;
      }
      case ELEM_REAL: {
        float *floatData = (float *)arrayStruct->data;
        printf("%g", (double)floatData[i]);
        break;
      }
      case ELEM_CHAR: {
        int8_t *charData = (int8_t *)arrayStruct->data;
        printf("%c", charData[i]);
        break;
      }
      case ELEM_BOOL: {
        int8_t *boolData = (int8_t *)arrayStruct->data;
        printf("%c", boolData[i] ? 'T' : 'F');
        break;
      }
      }
    }
  }

  printf("]");
}
