#include <hikari/unlocker.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wlr/types/wlr_input_device.h>

#include <hikari/keyboard.h>
#include <hikari/server.h>
#include <hikari/utf8.h>

static const size_t BUFFER_SIZE = 1024;

static char input_buffer[BUFFER_SIZE];
static int cursor = 0;
static int locker_pipe[2][2] = { { -1, -1 }, { -1, -1 } };

void
hikari_unlocker_init(void)
{
  mlock(input_buffer, BUFFER_SIZE);
  bzero(input_buffer, BUFFER_SIZE);
}

void
hikari_unlocker_start(void)
{
  cursor = 0;
  bzero(input_buffer, BUFFER_SIZE);

  pipe(locker_pipe[0]);
  pipe(locker_pipe[1]);

  pid_t locker = fork();

  if (locker == 0) {
    close(locker_pipe[0][1]);
    close(locker_pipe[1][0]);
    close(0);
    close(1);
    dup2(locker_pipe[0][0], 0);
    dup2(locker_pipe[1][1], 1);
    execl("/bin/sh", "/bin/sh", "-c", "hikari-unlocker", NULL);
    exit(0);
  } else {
    close(locker_pipe[0][0]);
    close(locker_pipe[1][1]);
  }
}

static void
put_char(uint32_t codepoint)
{
  size_t length = utf8_chsize(codepoint);

  if (cursor + length < BUFFER_SIZE) {
    utf8_encode(&input_buffer[cursor], length, codepoint);
    cursor += length;
  }
}

static void
delete_char(void)
{
  if (cursor == 0) {
    return;
  }

  input_buffer[--cursor] = '\0';
}

static void
submit_password(void)
{
  size_t password_length = strnlen(input_buffer, 1023) + 1;
  bool success = false;

  write(locker_pipe[0][1], input_buffer, password_length);
  bzero(input_buffer, BUFFER_SIZE);
  cursor = 0;
  read(locker_pipe[1][0], &success, sizeof(bool));

  if (success) {
    int status;
    hikari_server_unlock();
    close(locker_pipe[1][0]);
    close(locker_pipe[0][1]);
    wait(&status);
  }
}

void
hikari_unlocker_key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  if (event->state == WLR_KEY_PRESSED) {
    const xkb_keysym_t *syms;
    uint32_t keycode = event->keycode + 8;
    uint32_t codepoint;

    int nsyms = xkb_state_key_get_syms(
        keyboard->device->keyboard->xkb_state, keycode, &syms);

    for (int i = 0; i < nsyms; i++) {
      switch (syms[i]) {
        case XKB_KEY_Caps_Lock:
        case XKB_KEY_Shift_L:
        case XKB_KEY_Shift_R:
        case XKB_KEY_Control_L:
        case XKB_KEY_Control_R:
        case XKB_KEY_Meta_L:
        case XKB_KEY_Meta_R:
        case XKB_KEY_Alt_L:
        case XKB_KEY_Alt_R:
        case XKB_KEY_Super_L:
        case XKB_KEY_Super_R:
          break;

        case XKB_KEY_BackSpace:
          delete_char();
          break;

        case XKB_KEY_Return:
          submit_password();
          break;

        default:
          codepoint = xkb_state_key_get_utf32(
              keyboard->device->keyboard->xkb_state, keycode);

          if (codepoint) {
            put_char(codepoint);
          }
          break;
      }
    }
  }
}

void
hikari_unlocker_fini(void)
{
  munlock(input_buffer, BUFFER_SIZE);
}
