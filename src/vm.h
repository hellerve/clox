#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

#define STACK_MAX 256

typedef struct {
  chunk* c;
  uint8_t* ip;
  value stack[STACK_MAX];
  value* stack_top;
} vm;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} interpret_result;

vm* init_vm();
void free_vm(vm*);

interpret_result interpret(vm*, chunk*);

void push(vm*, value);
value pop(vm*);


#endif
