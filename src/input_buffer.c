#include <hikari/input_buffer.h>

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <hikari/utf8.h>

static bool
is_empty(struct hikari_input_buffer *input_buffer)
{
  size_t pos = input_buffer->pos;

  assert(pos == 0 ? input_buffer->buffer[0] == '\0' : true);

  return pos == 0;
}

static bool
current_char(struct hikari_input_buffer *input_buffer, char *current)
{
  size_t pos = input_buffer->pos;

  if (is_empty(input_buffer)) {
    return false;
  } else {
    *current = input_buffer->buffer[pos - 1];
    return true;
  }
}

struct hikari_input_buffer *
hikari_input_buffer_init(
    struct hikari_input_buffer *input_buffer, char *content)
{
  strncpy(input_buffer->buffer, content, sizeof(input_buffer->buffer) - 1);

  input_buffer->pos =
      strnlen(input_buffer->buffer, sizeof(input_buffer->buffer));

  return input_buffer;
}

void
hikari_input_buffer_add_char(
    struct hikari_input_buffer *input_buffer, char input)
{
  if (input_buffer->pos == sizeof(input_buffer->buffer) - 1) {
    return;
  }

  input_buffer->buffer[input_buffer->pos] = input;
  input_buffer->pos++;
}

void
hikari_input_buffer_add_utf32_char(
    struct hikari_input_buffer *input_buffer, uint32_t codepoint)
{
  size_t length = utf8_chsize(codepoint);

  if (input_buffer->pos + length > sizeof(input_buffer->buffer) - 1) {
    return;
  }

  utf8_encode(&input_buffer->buffer[input_buffer->pos], length, codepoint);
  input_buffer->pos += length;
}

void
hikari_input_buffer_remove_char(struct hikari_input_buffer *input_buffer)
{
  if (is_empty(input_buffer)) {
    return;
  }

  input_buffer->pos--;
  input_buffer->buffer[input_buffer->pos] = '\0';
}

void
hikari_input_buffer_remove_word(struct hikari_input_buffer *input_buffer)
{
  if (is_empty(input_buffer)) {
    return;
  }

  for (;;) {
    hikari_input_buffer_remove_char(input_buffer);

    char current;
    bool has_char = current_char(input_buffer, &current);

    if (!has_char || isspace(current)) {
      break;
    }
  }
}

void
hikari_input_buffer_clear(struct hikari_input_buffer *input_buffer)
{
  memset(input_buffer->buffer, 0, sizeof(input_buffer->buffer));
  input_buffer->pos = 0;
}

void
hikari_input_buffer_replace(
    struct hikari_input_buffer *input_buffer, char *content)
{
  hikari_input_buffer_clear(input_buffer);
  char *c = content;

  for (int i = 0; i < sizeof(input_buffer->buffer) && *c != '\0'; i++) {
    hikari_input_buffer_add_char(input_buffer, *c);
    c++;
  }
}

char *
hikari_input_buffer_read(struct hikari_input_buffer *input_buffer)
{
  return input_buffer->buffer;
}
