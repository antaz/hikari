#if !defined(HIKARI_UTF8_H)
#define HIKARI_UTF8_H

#include <stdint.h>

static inline size_t
utf8_chsize(uint32_t codepoint)
{
  if (codepoint < 0x80) {
    return 1;
  } else if (codepoint < 0x800) {
    return 2;
  } else if (codepoint < 0x10000) {
    return 3;
  }
  return 4;
}

static inline void
utf8_encode(char *buffer, size_t length, uint32_t codepoint)
{
  uint8_t first;

  if (codepoint < 0x80) {
    first = 0;
  } else if (codepoint < 0x800) {
    first = 0xc0;
  } else if (codepoint < 0x10000) {
    first = 0xe0;
  } else {
    first = 0xf0;
  }

  for (size_t i = length - 1; i > 0; --i) {
    buffer[i] = (codepoint & 0x3f) | 0x80;
    codepoint >>= 6;
  }

  buffer[0] = codepoint | first;
}

#endif
