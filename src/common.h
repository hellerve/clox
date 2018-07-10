#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULO,
  OP_SHIFTLEFT,
  OP_SHIFTRIGHT,
  OP_BITNOT,
  OP_BITOR,
  OP_BITXOR,
  OP_BITAND,
  OP_NEGATE,
  OP_RETURN,
} op_code;

#endif
