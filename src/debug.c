#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassemble_chunk(chunk* c, const char* name) {
  int i = 0;

  printf("== %s ==\n", name);

  while (i < c->count) {
    i = disassemble_instruction(c, i);
  }

  printf("========\n");
}

static int simple_instruction(const char* name, int offs) {
  printf("%s\n", name);
  return offs + 1;
}

void print_value(value v) {
  printf("%g", v);
}

static int constant_instruction(const char* name, chunk* c, int offs) {
  uint8_t constant = c->code[offs+1];
  printf("%-16s %4d '", name, constant);
  print_value(c->constants.values[constant]);
  printf("'\n");
  return offs + 2;
}

static int long_constant_instruction(const char* name, chunk* c, int offs) {
  uint32_t constant = c->code[offs+1] + (c->code[offs+2]<<8) + (c->code[offs+3]<<16);
  printf("%-16s %4d '", name, constant);
  print_value(c->constants.values[constant]);
  printf("'\n");
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

int disassemble_instruction(chunk* c, int offs) {
  uint8_t instruction = c->code[offs];
  int line = get_line(c->lines, c->lines_count, offs);
  int prev_line = get_line(c->lines, c->lines_count, offs-1);

  printf("%04d ", offs);
  if (line == prev_line) printf("   | ");
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
    default:
      printf("Unknown opcode %d\n", instruction);
      return offs + 1;
  }
}
