#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_RETURN,
} op_code;

#endif
