#ifndef hash_h
#define hash_h

#include "common.h"
#include "value.h"

typedef struct {
  obj_str* key;
  value val;
} entry;

typedef struct {
  int count;
  int capacity;
  entry* entries;
} table;

void init_table(table*);
bool table_set(table*, obj_str*, value);
bool table_get(table*, obj_str*, value*);
bool table_delete(table*, obj_str*);
void table_add_all(table*, table*);
obj_str* table_find_str(table*, const char*, int, uint32_t);
void free_table(table*);
#endif
