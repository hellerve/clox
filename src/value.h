#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
  BOOL,
  NIL,
  NUMBER,
} value_type;

typedef struct {
  value_type type;
  union {
    bool boolean;
    double num;
  } as;
}value;

#define BOOL_VAL(v)   ((value){ BOOL, { .boolean = v } })
#define NIL_VAL           ((value){ NIL, { .num = 0 } })
#define NUMBER_VAL(v) ((value){ NUMBER, { .num = v } })

#define AS_BOOL(v)    ((v).as.boolean)
#define AS_NUMBER(v)  ((v).as.num)

#define IS_BOOL(v)    ((v).type == BOOL)
#define IS_NIL(v)     ((v).type == NIL)
#define IS_NUMBER(v)  ((v).type == NUMBER)

typedef struct {
  int capacity;
  int count;
  value* values;
} value_array;

void init_value_array(value_array*);
void write_value_array(value_array*, value);
void free_value_array(value_array*);

bool values_equal(value, value);

#endif
