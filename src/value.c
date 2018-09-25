#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"

void init_value_array(value_array* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void write_value_array(value_array* array, value v) {
  if (array->capacity < array->count + 1) {
    int oldc = array->capacity;
    array->capacity = GROW_CAPACITY(oldc);
    array->values = GROW_ARRAY(array->values, value, oldc, array->capacity);
  }

  array->values[array->count] = v;
  array->count++;
}

void free_value_array(value_array* array) {
  FREE_ARRAY(value, array->values, array->capacity);
  init_value_array(array);
}

bool values_equal(value a, value b) {
  if (a.type != b.type) return false;

  switch (a.type) {
    case BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case NIL:    return true;
    case NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case CHAR:   return AS_CHAR(a) == AS_CHAR(b);
    case OBJ:
    {
      obj_str* a_str = AS_STRING(a);
      obj_str* b_str = AS_STRING(b);
      return a_str->len == b_str->len &&
          !memcmp(a_str->chars, b_str->chars, a_str->len);
    }
  }
}
