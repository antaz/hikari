#if !defined(HIKARI_INPUT_BUFFER_H)
#define HIKARI_INPUT_BUFFER_H

#include <stdint.h>
#include <stdlib.h>

struct hikari_input_buffer {
  char buffer[256];
  size_t pos;
};

struct hikari_input_buffer *
input_buffer_init(struct hikari_input_buffer *input_buffer, char *content);

void
hikari_input_buffer_add_char(
    struct hikari_input_buffer *input_buffer, char input);

void
hikari_input_buffer_add_utf32_char(
    struct hikari_input_buffer *input_buffer, uint32_t codepoint);

void
hikari_input_buffer_replace(
    struct hikari_input_buffer *input_buffer, char *content);

void
hikari_input_buffer_remove_char(struct hikari_input_buffer *input_buffer);

void
hikari_input_buffer_remove_word(struct hikari_input_buffer *input_buffer);

void
hikari_input_buffer_clear(struct hikari_input_buffer *input_buffer);

char *
hikari_input_buffer_read(struct hikari_input_buffer *input_buffer);

#endif
