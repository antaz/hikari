#include <hikari/memory.h>

#include <stdlib.h>

void *
hikari_malloc(size_t size)
{
  return malloc(size);
}

void *
hikari_calloc(size_t number, size_t size)
{
  return calloc(number, size);
}

void
hikari_free(void *ptr)
{
  return free(ptr);
}
