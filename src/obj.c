#include <stddef.h>
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

static obj_str* allocate_str(vm* cvm, char* chars, int len, uint32_t hash) {
  obj_str* string = (obj_str*)allocate_obj(cvm, offsetof(obj_str, chars)+len+1, STRING);
  string->len = len;
  for (int i = 0; i < len; i++) string->chars[i] = chars[i];
  string->chars[len] = '\0';
  string->hash = hash;

  table_set(&cvm->strings, string, NIL_VAL);

  return string;
}

static uint32_t hash_str(const char* key, int len) {
  uint32_t hash = 2166136261u;

  for (int i = 0; i < len; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }

  return hash;
}

obj_str* take_str(void* cvm, char* chars, int len) {
  uint32_t hash = hash_str(chars, len);
  obj_str* interned = table_find_str(&((vm*)cvm)->strings, chars, len, hash);
  if (interned) {
    FREE_ARRAY(char, chars, len);
    return interned;
  }
  return allocate_str((vm*)cvm, chars, len, hash);
}

obj_str* copy_str(void* cvm, const char* chars, int len) {
  uint32_t hash = hash_str(chars, len);
  obj_str* interned = table_find_str(&((vm*)cvm)->strings, chars, len, hash);
  if (interned) return interned;
  return allocate_str((vm*)cvm, (char*)chars, len, hash);
}
