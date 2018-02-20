#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  int lines_capacity;
  int lines_count;
  value_array constants;
} chunk;

void init_chunk(chunk*);
void write_chunk(chunk*, uint8_t byte, int line);
void free_chunk(chunk*);
int add_constant(chunk* chunk, value value);
void write_constant(chunk* chunk, value value, int line);

#endif
