#include <stdlib.h>

#include "common.h"
#include "memory.h"

void* reallocate(void* previous, size_t olds, size_t news) {
  return realloc(previous, news);
}
