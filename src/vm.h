#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "hash.h"

#define STACK_MAX 256

typedef struct {
  chunk* c;
  uint8_t* ip;
  value stack[STACK_MAX];
  value* stack_top;
  table globals;
  table strings;

  obj* objs;
} vm;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} interpret_result;

vm* init_vm();
void free_vm(vm*);

interpret_result interpret(vm*, const char*);

void push(vm*, value);
value pop(vm*);


#endif
