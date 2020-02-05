#if !defined(HIKARI_MEMORY_H)
#define HIKARI_MEMORY_H

#include <stdlib.h>

void *
hikari_malloc(size_t size);

void *
hikari_calloc(size_t number, size_t size);

void
hikari_free(void *ptr);

#endif
