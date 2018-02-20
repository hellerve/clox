#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double value;

typedef struct {
  int capacity;
  int count;
  value* values;
} value_array;

void init_value_array(value_array*);
void write_value_array(value_array*, value);
void free_value_array(value_array*);

#endif
