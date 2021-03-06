#ifndef clox_memory_h
#define clox_memory_h

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type)*(count))

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(previous, type, oldc, count) \
    (type*)reallocate(previous, sizeof(type) * (oldc), sizeof(type) * (count))

#define FREE_ARRAY(type, p, oldc) reallocate(p, sizeof(type) * (oldc), 0)
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void* reallocate(void*, size_t, size_t);

#endif
