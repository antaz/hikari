#include <hikari/binding_config.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <linux/input-event-codes.h>

#include <wlr/types/wlr_keyboard.h>

static bool
parse_modifier_mask(const char *str, uint8_t *result, const char **remaining)
{
  size_t len = strlen(str);
  uint8_t mask = 0;
  int pos;

  for (pos = 0; pos < len; pos++) {
    char c = str[pos];
    if (c == '-' || c == '+') {
      break;
    } else if (c == 'L') {
      mask |= WLR_MODIFIER_LOGO;
    } else if (c == 'S') {
      mask |= WLR_MODIFIER_SHIFT;
    } else if (c == 'A') {
      mask |= WLR_MODIFIER_ALT;
    } else if (c == 'C') {
      mask |= WLR_MODIFIER_CTRL;
    } else if (c == '5') {
      mask |= WLR_MODIFIER_MOD5;
    } else if (c == '0') {
      // do nothing
    } else {
      fprintf(stderr,
          "configuration error: unknown modifier \"%c\" in \"%s\"\n",
          c,
          str);
      return false;
    }
  }

  *result = mask;
  if (remaining != NULL) {
    *remaining = str + pos;
  }

  return true;
}

bool
hikari_binding_config_key_parse(
    struct hikari_binding_config_key *binding_key, const char *str)
{
  bool success = false;
  const char *remaining;

  if (!parse_modifier_mask(str, &binding_key->modifiers, &remaining)) {
    goto done;
  }

  if (*remaining == '-') {
    errno = 0;
    const long value = strtol(remaining + 1, NULL, 10);
    if (errno != 0 || value < 0 || value > UINT32_MAX) {
      fprintf(stderr,
          "configuration error: failed to parse keycode \"%s\"\n",
          remaining + 1);
      goto done;
    }

    binding_key->type = HIKARI_ACTION_BINDING_KEY_KEYCODE;
    binding_key->value.keycode = (uint32_t)value - 8;
  } else if (*remaining == '+') {
    xkb_keysym_t value =
        xkb_keysym_from_name(remaining + 1, XKB_KEYSYM_CASE_INSENSITIVE);
    if (value == XKB_KEY_NoSymbol) {
      fprintf(stderr,
          "configuration error: unknown key symbol \"%s\"\n",
          remaining + 1);
      goto done;
    }

    binding_key->type = HIKARI_ACTION_BINDING_KEY_KEYSYM;
    binding_key->value.keysym = value;
  } else {
    goto done;
  }

  success = true;

done:

  return success;
}

bool
hikari_binding_config_button_parse(
    struct hikari_binding_config_key *binding_key, const char *str)
{
  bool success = false;
  const char *remaining;

  if (!parse_modifier_mask(str, &binding_key->modifiers, &remaining)) {
    goto done;
  }

  if (*remaining == '+') {
    binding_key->type = HIKARI_ACTION_BINDING_KEY_KEYCODE;

    remaining++;

    if (!strcmp(remaining, "left")) {
      binding_key->value.keycode = BTN_LEFT;
    } else if (!strcmp(remaining, "right")) {
      binding_key->value.keycode = BTN_RIGHT;
    } else if (!strcmp(remaining, "middle")) {
      binding_key->value.keycode = BTN_MIDDLE;
    } else if (!strcmp(remaining, "side")) {
      binding_key->value.keycode = BTN_SIDE;
    } else if (!strcmp(remaining, "extra")) {
      binding_key->value.keycode = BTN_EXTRA;
    } else if (!strcmp(remaining, "forward")) {
      binding_key->value.keycode = BTN_FORWARD;
    } else if (!strcmp(remaining, "back")) {
      binding_key->value.keycode = BTN_BACK;
    } else if (!strcmp(remaining, "task")) {
      binding_key->value.keycode = BTN_TASK;
    } else {
      fprintf(stderr,
          "configuration error: unknown mouse button \"%s\"\n",
          remaining);
      goto done;
    }
  } else if (*remaining == '-') {
    binding_key->type = HIKARI_ACTION_BINDING_KEY_KEYCODE;

    remaining++;

    errno = 0;
    const long value = strtol(remaining, NULL, 10);
    if (errno != 0 || value < 0 || value > UINT32_MAX) {
      fprintf(stderr,
          "configuration error: failed to parse mouse binding \"%s\"\n",
          remaining);
      goto done;
    }
  } else {
    goto done;
  }

  success = true;

done:

  return success;
}
