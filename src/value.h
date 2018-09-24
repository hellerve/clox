#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
  STRING,
} obj_type;

typedef struct obj {
  obj_type type;
  struct obj* next;
} obj;

typedef struct obj_str {
  obj o;
  int len;
  char chars[];
} obj_str;

typedef enum {
  BOOL,
  NIL,
  NUMBER,
  OBJ,
} value_type;

typedef struct {
  value_type type;
  union {
    bool boolean;
    double num;
    obj* o;
  } as;
}value;

#define BOOL_VAL(v)   ((value){ BOOL, { .boolean = v } })
#define NIL_VAL       ((value){ NIL, { .num = 0 } })
#define NUMBER_VAL(v) ((value){ NUMBER, { .num = v } })
#define OBJ_VAL(v)    ((value){ OBJ, { .o = (obj*)v }})

#define AS_BOOL(v)    ((v).as.boolean)
#define AS_NUMBER(v)  ((v).as.num)
#define AS_OBJ(v)     ((v).as.o)

#define IS_BOOL(v)    ((v).type == BOOL)
#define IS_NIL(v)     ((v).type == NIL)
#define IS_NUMBER(v)  ((v).type == NUMBER)
#define IS_OBJ(v)  ((v).type == OBJ)

typedef struct {
  int capacity;
  int count;
  value* values;
} value_array;

void init_value_array(value_array*);
void write_value_array(value_array*, value);
void free_value_array(value_array*);

bool values_equal(value, value);

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

#define IS_STRING(v) is_obj_type(v, STRING)

#define AS_STRING(v)  ((obj_str*)AS_OBJ(v))
#define AS_CSTRING(v) (((obj_str*)AS_OBJ(v))->chars)

static inline bool is_obj_type(value v, obj_type type) {
  return IS_OBJ(v) && AS_OBJ(v)->type == type;
}

obj_str* copy_str(void*, const char*, int);
void print_obj(value);
obj_str* take_str(void*, char*, int);

#endif
