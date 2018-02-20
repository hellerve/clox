#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "memory.h"

void init_chunk(chunk* c) {
  c->count = 0;
  c->capacity = 0;
  c->code = NULL;
  c->lines = NULL;
  c->lines_count = 0;
  c->lines_capacity = 0;
  init_value_array(&c->constants);
}

void encode_line(chunk* c, int line) {
  if (c->lines && c->lines[c->lines_count-1] == line) {
    c->lines[c->lines_count-2] += 1;
    return;
  }

  if (c->lines_capacity < c->lines_count + 1) {
    int oldc = c->lines_capacity;
    c->lines_capacity = GROW_CAPACITY(c->lines_capacity);
    c->lines = GROW_ARRAY(c->lines, int, oldc, c->lines_capacity);
  }
  c->lines[c->lines_count] = 1;
  c->lines[c->lines_count+1] = line;
  c->lines_count += 2;
}

void write_chunk(chunk* c, uint8_t byte, int line) {
  if (c->capacity < c->count + 1) {
    int oldc = c->capacity;
    c->capacity = GROW_CAPACITY(oldc);
    c->code = GROW_ARRAY(c->code, uint8_t, oldc, c->capacity);
  }

  c->code[c->count] = byte;
  encode_line(c, line);
  c->count++;
}

void write_constant(chunk* c, value v, int line) {
  int offs = add_constant(c, v);

  if (c->capacity < c->count + 4) {
    int oldc = c->capacity;
    c->capacity = GROW_CAPACITY(oldc);
    c->code = GROW_ARRAY(c->code, uint8_t, oldc, c->capacity);
  }

  if (offs < 256) {
    c->code[c->count] = OP_CONSTANT;
    encode_line(c, line);
    c->code[c->count+1] = offs;
    encode_line(c, line);
    c->count += 2;
  } else {
    c->code[c->count] = OP_CONSTANT_LONG;
    encode_line(c, line);
    c->code[c->count+1] = offs & 0xff;
    encode_line(c, line);
    c->code[c->count+2] = (offs & 0xff00) >> 8;
    encode_line(c, line);
    c->code[c->count+3] = (offs & 0xff0000) >> 16;
    encode_line(c, line);
    c->count += 4;
  }
}

void free_chunk(chunk* c) {
  FREE_ARRAY(uint8_t, c->code, c->capacity);
  FREE_ARRAY(int, c->lines, c->capacity);
  free_value_array(&c->constants);
  init_chunk(c);
}

int add_constant(chunk* c, value value) {
  write_value_array(&c->constants, value);
  return c->constants.count - 1;
}
