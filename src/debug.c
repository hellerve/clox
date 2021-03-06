#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassemble_chunk(chunk* c, const char* name) {
  int i = 0;

  printf("== %s ==\n", name);

  while (i < c->count) {
    i = disassemble_instruction(c, i);
  }

  puts("========");
}

static int simple_instruction(const char* name, int offs) {
  printf("%s\n", name);
  return offs + 1;
}

void print_value(value v) {
  switch (v.type) {
    case BOOL:   printf(AS_BOOL(v) ? "true" : "false"); break;
    case NIL:    printf("nil"); break;
    case NUMBER: printf("%g", AS_NUMBER(v)); break;
    case CHAR:   printf("%c", AS_CHAR(v)); break;
    case OBJ:    print_obj(v); break;
  }
}

static int constant_instruction(const char* name, chunk* c, int offs) {
  uint8_t constant = c->code[offs+1];
  printf("%-16s %4d '", name, constant);
  print_value(c->constants.values[constant]);
  puts("'");
  return offs + 2;
}

static int byte_instruction(const char* name, chunk* c, int offs) {
  uint8_t slot = c->code[offs+1];
  printf("%-16s %4d\n", name, slot);
  return offs+2;
}

static int long_constant_instruction(const char* name, chunk* c, int offs) {
  uint32_t constant = c->code[offs+1] + (c->code[offs+2]<<8) + (c->code[offs+3]<<16);
  printf("%-16s %4d '", name, constant);
  print_value(c->constants.values[constant]);
  puts("'");
  return offs + 4;
}

int get_line(int* lines, int count, int offs) {
  if (offs < 0) return -1;
  offs += 1;
  int i = 0;
  int offs_passed = 0;

  while (offs > offs_passed) {
    offs_passed += lines[i];
    i += 2;
  }
  return i ? lines[i-1] : 1;
}

static int jump_instruction(const char* name, int sign, chunk* c, int offs) {
  uint16_t jump = (uint16_t)(c->code[offs+1]<<8);
  jump |= c->code[offs+2];
  printf("%-16s %4d -> %d\n", name, offs, offs+3+sign*jump);
  return offs+3;
}

int disassemble_instruction(chunk* c, int offs) {
  uint8_t instruction = c->code[offs];
  int line = get_line(c->lines, c->lines_count, offs);
  int prev_line = get_line(c->lines, c->lines_count, offs-1);

  printf("%04d ", offs);
  if (line == prev_line) fputs("   | ", stdout);
  else printf("%4d ", line);

  switch (instruction) {
    case OP_RETURN:
      return simple_instruction("return", offs);
    case OP_CONSTANT:
      return constant_instruction("constant", c, offs);
    case OP_CONSTANT_LONG:
      return long_constant_instruction("long constant", c, offs);
    case OP_ADD:
      return simple_instruction("add", offs);
    case OP_SUBTRACT:
      return simple_instruction("subtract", offs);
    case OP_MULTIPLY:
      return simple_instruction("multiply", offs);
    case OP_DIVIDE:
      return simple_instruction("divide", offs);
    case OP_NEGATE:
      return simple_instruction("negate", offs);
    case OP_NIL:
      return simple_instruction("nil", offs);
    case OP_TRUE:
      return simple_instruction("true", offs);
    case OP_FALSE:
      return simple_instruction("false", offs);
    case OP_NOT:
      return simple_instruction("not", offs);
    case OP_EQUAL:
      return simple_instruction("eq", offs);
    case OP_GREATER:
      return simple_instruction("gt", offs);
    case OP_LESS:
      return simple_instruction("lt", offs);
    case OP_PRINT:
      return simple_instruction("print", offs);
    case OP_POP:
      return simple_instruction("pop", offs);
    case OP_DEFINE_GLOBAL:
      return constant_instruction("define global", c, offs);
    case OP_GET_GLOBAL:
      return constant_instruction("get global", c, offs);
    case OP_SET_GLOBAL:
      return constant_instruction("set global", c, offs);
    case OP_GET_LOCAL:
      return byte_instruction("get local", c, offs);
    case OP_SET_LOCAL:
      return byte_instruction("set local", c, offs);
    case OP_JUMP:
      return jump_instruction("jump", 1, c, offs);
    case OP_LOOP:
      return jump_instruction("loop", -1, c, offs);
    case OP_JUMP_IF_FALSE:
      return jump_instruction("jump if false", 1, c, offs);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offs + 1;
  }
}

void print_trace(vm* cvm) {
    fputs("          ", stdout);
    for (value* slot = cvm->stack; slot < cvm->stack_top; slot++) {
      fputs("[ ", stdout);
      print_value(*slot);
      fputs(" ]", stdout);
    }
    puts("");
    disassemble_instruction(cvm->c, (int)(cvm->ip - cvm->c->code));
  }
