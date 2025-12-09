#include "run_time_errors.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int32_t ipow_019addc8_6352_7de5_8629_b0688522175f(int32_t base, int32_t exp) {
  int32_t result = 1;
  for (int32_t i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

int printf_019ae38d_3df3_74a3_b276_d9a9f7a8008b(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vprintf(format, args);
  va_end(args);
  return result;
}

int scanf_019ae392_2fe0_72fc_ad1e_94bb9c5662c0(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vscanf(format, args);
  va_end(args);
  return result;
}

void *malloc_019b1cf2_3c2e_4f9f_a8d1_b2c5e7f0c123(size_t size) { return malloc(size); }

void free_019b1cf2_3c2e_4f9f_a8d1_b2c5e7f0c124(void *ptr) { free(ptr); }

int snprintf_019b1cf2_3c2e_4f9f_a8d1_b2c5e7f0c125(char *str, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf(str, size, format, args);
  va_end(args);
  return result;
}

void throwDivisionByZeroError_019addc8_a29b_740a_9b09_8a712296bc1a() {
  MathError("Division by zero");
}

void throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c() {
  SizeError("Array size mismatch");
}
void throwVectorSizeError_019addc9_1a57_7674_b3dd_79d0624d2029() {
  SizeError("Vector size mismatch");
}
void throwArrayIndexError_019ae3a1_54f9_7452_b095_6faaebe8aa2e() {
  IndexError("Array index out of range");
}
void throwStrideError_a2beb751_ff3b_4d60_aefb_60f92ff9f4be() { StrideError("Array stride error"); }

enum ElementType { ELEM_INT = 0, ELEM_REAL = 1, ELEM_CHAR = 2, ELEM_BOOL = 3, ELEM_ARRAY = 4 };

typedef struct {
  int32_t size;
  void *data;
  int8_t is2D; // boolean
} ArrayStruct;

void printString_d526a5bb_a01a_4579_9d33_c725c674e1c5(ArrayStruct *vectorStruct) {
  int8_t *charData = (int8_t *)vectorStruct->data;
  for (int32_t i = 0; i < vectorStruct->size; i++) {
    if (charData[i] == '\0') {
      break; // Stop at null terminator for strings
    }
    printf("%c", charData[i]);
  }
}

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
