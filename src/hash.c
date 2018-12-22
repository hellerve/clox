#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "hash.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(table* t) {
  t->count = 0;
  t->capacity = 0;
  t->entries = NULL;
}

static entry* find_entry(entry* entries, int capacity, obj_str* key) {
  uint32_t i = key->hash % capacity;
  entry* tombstone = NULL;
  while(1) {
    entry* e = entries+i;

    if (e->key == key) {
      return e;
    } else if (!e->key) {
      if (IS_NIL(e->val)) return tombstone ? tombstone : e;
      else if (!tombstone) tombstone = e;
    }

    i = (i+1) % capacity;
  }
}

static void adjust_capacity(table* t, int cap) {
  entry* entries = ALLOCATE(entry, cap);

  for (int i = 0; i < cap; i++) {
    entries[i].key = NULL;
    entries[i].val = NIL_VAL;
  }

  t->count = 0;
  for (int i = 0; i < t->capacity; i++) {
    entry* e = t->entries+i;
    if (!e->key) continue;

    entry* d = find_entry(entries, cap, e->key);
    d->key = e->key;
    d->val = e->val;
    t->count++;
  }

  FREE_ARRAY(value, t->entries, t->capacity);
  t->entries = entries;
  t->capacity = cap;
}

bool table_set(table* t, obj_str* key, value v) {
  if (t->count+1 > t->capacity*TABLE_MAX_LOAD) adjust_capacity(t, GROW_CAPACITY(t->capacity));
  entry* e = find_entry(t->entries, t->capacity, key);
  bool is_new_key = !e->key;
  e->key = key;
  e->val = v;

  if (is_new_key) t->count++;
  return is_new_key;
}

bool table_get(table* t, obj_str* key, value* v) {
  if (!t->entries) return false;

  entry* e = find_entry(t->entries, t->capacity, key);
  if (!e->key) return false;

  *v = e->val;
  return true;
}

bool table_delete(table* t, obj_str* key) {
  if (!t->count) return false;

  entry* e = find_entry(t->entries, t->capacity, key);

  if (!e->key) return false;

  e->key = NULL;
  e->val = BOOL_VAL(true);

  return true;
}

void table_add_all(table* from, table* to) {
  for (int i = 0; i < from->capacity; i++) {
    entry* e = from->entries+i;
    if (e->key) table_set(to, e->key, e->val);
  }
}

void free_table(table* t) {
  FREE_ARRAY(entry, t->entries, t->capacity);
  init_table(t);
}

obj_str* table_find_str(table* t, const char* chars, int len, uint32_t hash) {
  if (!t->entries) return NULL;

  uint32_t i = hash % t->capacity;

  while (1) {
    entry* e = t->entries+i;

    if (!e->key) return NULL;
    if (e->key->len == len && !memcmp(e->key->chars, chars, len)) return e->key;

    i = (i + 1) % t->capacity;
  }

  return NULL;
}
