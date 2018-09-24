#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(vm, t, obj_t) (t*)allocate_obj(vm, sizeof(t), obj_t)

void print_obj(value v) {
  switch (OBJ_TYPE(v)) {
    case STRING:
      printf("%s", AS_CSTRING(v));
      break;
  }
}

static obj* allocate_obj(vm* cvm, size_t size, obj_type type) {
  obj* o = (obj*)reallocate(NULL, 0, size);
  o->type = type;
  o->next = cvm->objs;
  cvm->objs = o;
  return o;
}

static obj_str* allocate_str(vm* cvm, char* chars, int length) {
  obj_str* string = ALLOCATE_OBJ(cvm, obj_str, STRING);
  string->len = length;
  string->chars = chars;

  return string;
}

obj_str* take_str(void* cvm, char* chars, int len) {
  return allocate_str((vm*)cvm, chars, len);
}

obj_str* copy_str(void* cvm, const char* chars, int len) {
  char* heap_chars = ALLOCATE(char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0';

  return allocate_str((vm*)cvm, heap_chars, len);
}
