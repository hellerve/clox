#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"
#include "vm.h"

void disassemble_chunk(chunk*, const char*);
int disassemble_instruction(chunk*, int);
void print_value(value);
void print_trace(vm*);

#endif
