#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

void reset_stack(vm* cvm) {
  cvm->stack_top = cvm->stack;
}

vm* init_vm() {
  vm* res = malloc(sizeof(vm));
  reset_stack(res);
  return res;
}

void free_vm(vm* cvm) {
  free(cvm);
}

void push(vm* cvm, value v) {
  *cvm->stack_top = v;
  cvm->stack_top++;
}

value pop(vm* cvm) {
  return *(--cvm->stack_top);
}

static interpret_result run(vm* cvm) {
#define binary_op(op) { \
      double b = pop(cvm); \
      double a = pop(cvm); \
      push(cvm, a op b); \
    }
#define read_byte() (*cvm->ip++)
#define read_constant() (cvm->c->constants.values[read_byte()])
#define read_long_constant() (cvm->c->constants.values[read_byte() + (read_byte()<<8) + (read_byte()<<16)])
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (value* slot = cvm->stack; slot < cvm->stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(cvm->c, (int)(cvm->ip - cvm->c->code));
#endif
    uint8_t instruction;
    switch (instruction = read_byte()) {
      case OP_CONSTANT: {
        value constant = read_constant();
        push(cvm, constant);
        break;
      }
      case OP_CONSTANT_LONG: {
        value constant = read_long_constant();
        push(cvm, constant);
        break;
      }
      case OP_RETURN: {
        print_value(pop(cvm));
        printf("\n");
        return INTERPRET_OK;
      }
      case OP_ADD:      binary_op(+); break;
      case OP_SUBTRACT: binary_op(-); break;
      case OP_MULTIPLY: binary_op(*); break;
      case OP_DIVIDE:   binary_op(/); break;
      case OP_NEGATE:   push(cvm, -pop(cvm)); break;
    }
  }
#undef binary_op
#undef read_byte
#undef read_constant
#undef read_long_constant
}

interpret_result interpret(vm* cvm, chunk* c) {
  cvm->c = c;
  cvm->ip = cvm->c->code;

  return run(cvm);
}
