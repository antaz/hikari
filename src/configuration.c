#include <hikari/configuration.h>

#include <errno.h>

#include <ucl.h>

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_cursor.h>

#include <hikari/background.h>
#include <hikari/color.h>
#include <hikari/command.h>
#include <hikari/exec.h>
#include <hikari/keybinding.h>
#include <hikari/keyboard.h>
#include <hikari/layout.h>
#include <hikari/mark.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/pointer_config.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/split.h>
#include <hikari/tile.h>
#include <hikari/view.h>
#include <hikari/view_autoconf.h>
#include <hikari/workspace.h>

struct hikari_configuration hikari_configuration;

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

static bool
parse_container(
    const ucl_object_t *container_obj, struct hikari_split **container)
{
  bool success = false;
  struct hikari_container *ret = NULL;
  layout_func_t layout_func = NULL;
  int64_t views = 1000;
  const ucl_object_t *cur;

  ucl_object_iter_t it = ucl_object_iterate_new(container_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "type")) {
      const char *container_type;
      if (!ucl_object_tostring_safe(cur, &container_type)) {
        fprintf(stderr,
            "configuration error: expected string for container \"type\"\n");
        goto done;
      }

      if (!strcmp(container_type, "vertical")) {
        layout_func = hikari_sheet_vertical_layout;
      } else if (!strcmp(container_type, "horizontal")) {
        layout_func = hikari_sheet_horizontal_layout;
      } else if (!strcmp(container_type, "full")) {
        layout_func = hikari_sheet_full_layout;
      } else if (!strcmp(container_type, "grid")) {
        layout_func = hikari_sheet_grid_layout;
      } else {
        fprintf(stderr,
            "configuration error: unexpected container \"type\" \"%s\"\n",
            container_type);
        goto done;
      }
    } else if (!strcmp(key, "views")) {
      if (!ucl_object_toint_safe(cur, &views)) {
        fprintf(stderr,
            "configuration error: expected integer for container \"views\"\n");
        goto done;
      }
    } else {
      fprintf(
          stderr, "configuration error: unknown container key \"%s\"\n", key);
      goto done;
    }
  }

  if (layout_func == NULL) {
    fprintf(stderr, "configuration error: container expects \"type\"\n");
    goto done;
  }

  ret = hikari_malloc(sizeof(struct hikari_container));
  hikari_container_init(ret, views, layout_func);

  success = true;

done:
  ucl_object_iterate_free(it);

  *container = (struct hikari_split *)ret;

  return success;
}

static bool
parse_vertical(const ucl_object_t *, struct hikari_split **);

static bool
parse_horizontal(const ucl_object_t *, struct hikari_split **);

static bool
parse_split(const ucl_object_t *split_obj, struct hikari_split **split)
{
  bool success = false;
  const ucl_object_t *cur;
  struct hikari_split *ret = NULL;
  ucl_object_iter_t it = ucl_object_iterate_new(split_obj);

  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (ret != NULL) {
      hikari_split_fini(ret);
      ret = NULL;
      fprintf(stderr,
          "configuration error: defining overlapping split \"%s\"\n",
          key);
      goto done;
    }

    if (!strcmp(key, "vertical")) {
      if (!parse_vertical(cur, &ret)) {
        fprintf(stderr,
            "configuration error: failed to parse \"vertical\" split\n");
        goto done;
      }
    } else if (!strcmp(key, "horizontal")) {
      if (!parse_horizontal(cur, &ret)) {
        fprintf(stderr,
            "configuration error: failed to parse \"horizontal\" split\n");
        goto done;
      }
    } else if (!strcmp(key, "container")) {
      if (!parse_container(cur, &ret)) {
        fprintf(stderr, "configuration error: failed to parse container\n");
        goto done;
      }
    } else {
      fprintf(stderr, "configuration error: unknown split \"%s\"\n", key);
      goto done;
    }
  }

  success = true;

done:
  *split = ret;
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_ratio(const ucl_object_t *ratio_obj, double *ratio)
{
  bool success = false;
  double ret = 0.5;
  if (ratio_obj != NULL) {
    if (!ucl_object_todouble_safe(ratio_obj, &ret)) {
      fprintf(
          stderr, "configuration error: expected float for split \"ratio\"\n");
      goto done;
    }
  }

  success = true;

done:
  *ratio = ret;

  return success;
}

#define PARSE_SPLIT(name, first, second)                                       \
  static bool parse_##name(                                                    \
      const ucl_object_t *name##_obj, struct hikari_split **name)              \
  {                                                                            \
    bool success = false;                                                      \
    struct hikari_##name##_split *ret = NULL;                                  \
    const ucl_object_t *cur;                                                   \
    struct hikari_split *first = NULL;                                         \
    struct hikari_split *second = NULL;                                        \
    double ratio = 0.5;                                                        \
                                                                               \
    ucl_object_iter_t it = ucl_object_iterate_new(name##_obj);                 \
                                                                               \
    while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {               \
      const char *key = ucl_object_key(cur);                                   \
                                                                               \
      if (!strcmp(key, "ratio")) {                                             \
        if (!parse_ratio(cur, &ratio)) {                                       \
          goto done;                                                           \
        }                                                                      \
      } else if (!strcmp(key, #first)) {                                       \
        if (!parse_split(cur, &first)) {                                       \
          fprintf(stderr,                                                      \
              "configuration error: invalid \"" #first "\" for \"" #name       \
              "\" split\n");                                                   \
          goto done;                                                           \
        }                                                                      \
      } else if (!strcmp(key, #second)) {                                      \
        if (!parse_split(cur, &second)) {                                      \
          if (first != NULL) {                                                 \
            hikari_split_fini(first);                                          \
          }                                                                    \
          fprintf(stderr,                                                      \
              "configuration error: invalid \"" #second "\" for \"" #name      \
              "\" split\n");                                                   \
          goto done;                                                           \
        }                                                                      \
      } else {                                                                 \
        fprintf(stderr,                                                        \
            "configuration error: unknown \"" #name "\" key \"%s\"\n",         \
            key);                                                              \
        goto done;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (first == NULL) {                                                       \
      fprintf(stderr,                                                          \
          "configuration error: missing \"" #first "\" for \"" #name           \
          "\" split\n");                                                       \
      goto done;                                                               \
    }                                                                          \
                                                                               \
    if (second == NULL) {                                                      \
      hikari_split_fini(first);                                                \
      fprintf(stderr,                                                          \
          "configuration error: missing \"" #second "\" for " #name            \
          " split\n");                                                         \
      goto done;                                                               \
    }                                                                          \
                                                                               \
    ret = hikari_malloc(sizeof(struct hikari_##name##_split));                 \
    hikari_##name##_split_init(ret, ratio, first, second);                     \
                                                                               \
    success = true;                                                            \
                                                                               \
  done:                                                                        \
    ucl_object_iterate_free(it);                                               \
                                                                               \
    *name = (struct hikari_split *)ret;                                        \
                                                                               \
    return success;                                                            \
  }

PARSE_SPLIT(vertical, left, right);
PARSE_SPLIT(horizontal, top, bottom);
#undef PARSE_SPLIT

static struct hikari_view_autoconf *
resolve_autoconf(struct hikari_configuration *configuration, const char *app_id)
{
  if (app_id != NULL) {
    struct hikari_view_autoconf *autoconf;
    wl_list_for_each (autoconf, &hikari_configuration.autoconfs, link) {
      if (!strcmp(autoconf->app_id, app_id)) {
        return autoconf;
      }
    }
  }

  return NULL;
}

void
hikari_configuration_resolve_view_autoconf(
    struct hikari_configuration *configuration,
    const char *app_id,
    struct hikari_view *view,
    struct hikari_sheet **sheet,
    struct hikari_group **group,
    int *x,
    int *y,
    bool *focus)
{
  struct hikari_view_autoconf *view_autoconf =
      resolve_autoconf(&hikari_configuration, app_id);

  if (view_autoconf != NULL) {
    *focus = view_autoconf->focus;

    if (view_autoconf->sheet_nr != -1) {
      *sheet = hikari_server.workspace->sheets + view_autoconf->sheet_nr;
    } else {
      *sheet = hikari_server.workspace->sheet;
    }

    if (view_autoconf->group_name != NULL &&
        strlen(view_autoconf->group_name) > 0) {
      *group = hikari_server_find_or_create_group(view_autoconf->group_name);
    } else {
      *group = (*sheet)->group;
    }

    if (view_autoconf->position.x != -1 && view_autoconf->position.y != -1) {
      *x = view_autoconf->position.x;
      *y = view_autoconf->position.y;
    } else {
      struct hikari_output *output = hikari_server.workspace->output;

      *x = hikari_server.cursor->x - output->geometry.x;
      *y = hikari_server.cursor->y - output->geometry.y;
    }

    if (view_autoconf->mark != NULL && view_autoconf->mark->view == NULL) {
      hikari_mark_set(view_autoconf->mark, view);
    }
  } else {
    struct hikari_output *output = hikari_server.workspace->output;

    *sheet = hikari_server.workspace->sheet;
    *group = (*sheet)->group;

    *x = hikari_server.cursor->x - output->geometry.x;
    *y = hikari_server.cursor->y - output->geometry.y;
  }
}

static char *
copy_in_config_string(const ucl_object_t *obj)
{
  const char *str;
  char *ret;

  bool success = ucl_object_tostring_safe(obj, &str);

  if (success) {
    size_t len = strlen(str);
    ret = hikari_malloc(len + 1);
    strcpy(ret, str);

    return ret;
  } else {
    fprintf(stderr, "configuration error: expected string\n");
    return NULL;
  }
}

static bool
parse_colorscheme(struct hikari_configuration *configuration,
    const ucl_object_t *colorscheme_obj)
{
  bool success = false;
  const ucl_object_t *cur;
  int64_t color;

  ucl_object_iter_t it = ucl_object_iterate_new(colorscheme_obj);

  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp("indicator_selected", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_selected, color);
    } else if (!strcmp("indicator_grouped", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_grouped, color);
    } else if (!strcmp("indicator_first", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_first, color);
    } else if (!strcmp("indicator_conflict", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_conflict, color);
    } else if (!strcmp("indicator_insert", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_insert, color);
    } else if (!strcmp("border_active", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->border_active, color);
    } else if (!strcmp("border_inactive", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->border_inactive, color);
    } else if (!strcmp("foreground", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->foreground, color);
    } else if (!strcmp("background", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->clear, color);
    } else {
      fprintf(stderr, "configuration error: unknown color key \"%s\"\n", key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_execute(
    struct hikari_configuration *configuration, const ucl_object_t *obj)
{
  bool success = false;
  const ucl_object_t *cur;
  const char *key;

  ucl_object_iter_t it = ucl_object_iterate_new(obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    key = ucl_object_key(cur);

    struct hikari_exec *execute = NULL;

    if (strlen(key) != 1 || !(key[0] >= 'a' && key[0] <= 'z')) {
      fprintf(stderr,
          "configuration error: invalid \"execute\" register \"%s\"\n",
          key);
      goto done;
    } else {
      int nr = key[0] - 'a';
      execute = &execs[nr];
    }

    assert(execute != NULL);

    char *command = copy_in_config_string(cur);

    if (command != NULL) {
      execute->command = command;
    } else {
      fprintf(stderr,
          "configuration error: invalid command \"%s\" for \"execute\" "
          "register \"%c\"\n",
          command,
          key[0]);
      goto done;
    }
    execute = NULL;
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_position(const ucl_object_t *position_obj, int *x, int *y)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(position_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "x")) {
      int64_t ret_x;
      if (!ucl_object_toint_safe(cur, &ret_x)) {
        fprintf(stderr,
            "configuration error: expected integer for \"x\"-coordinate\n");
        goto done;
      }

      *x = ret_x;
    } else if (!strcmp(key, "y")) {
      int64_t ret_y;
      if (!ucl_object_toint_safe(cur, &ret_y)) {
        fprintf(stderr,
            "configuration error: expected integer for \"y\"-coordinate\n");
        goto done;
      }

      *y = ret_y;
    } else {
      fprintf(stderr,
          "configuration error: unknown \"position\" key \"%s\"\n",
          key);
      goto done;
    }
  }

  if (*x == -1) {
    fprintf(stderr,
        "configuration error: missing \"x\"-coordinate in \"position\2\n");
    goto done;
  }

  if (*y == -1) {
    fprintf(stderr,
        "configuration error: missing \"y\"-coordinate in \"position\"\n");
    goto done;
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_autoconf(
    const ucl_object_t *autoconf_obj, struct hikari_view_autoconf **autoconf)
{
  bool success = false;
  (*autoconf)->group_name = NULL;
  (*autoconf)->sheet_nr = -1;
  (*autoconf)->mark = NULL;
  (*autoconf)->position.x = -1;
  (*autoconf)->position.y = -1;
  (*autoconf)->focus = false;

  ucl_object_iter_t it = ucl_object_iterate_new(autoconf_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "group")) {
      char *group_name = copy_in_config_string(cur);
      if (group_name == NULL) {
        fprintf(stderr,
            "configuration error: expected string for \"autoconf\" "
            "\"group\"\n");
        goto done;
      } else if (strlen(group_name) == 0) {
        fprintf(stderr,
            "configuration error: expected non-empty string for \"autoconf\" "
            "\"group\"\n");
        goto done;
      }

      (*autoconf)->group_name = group_name;

    } else if (!strcmp(key, "sheet")) {
      int64_t sheet_nr;
      if (!ucl_object_toint_safe(cur, &sheet_nr)) {
        fprintf(stderr,
            "configuration error: expected integer for \"autoconf\" "
            "\"sheet\"\n");
        goto done;
      }

      (*autoconf)->sheet_nr = sheet_nr;
    } else if (!strcmp(key, "mark")) {
      const char *mark_name;

      if (!ucl_object_tostring_safe(cur, &mark_name)) {
        fprintf(stderr,
            "configuration error: expected string for \"autoconf\" \"mark\"");
        goto done;
      }

      if (strlen(mark_name) != 1) {
        fprintf(stderr,
            "configuration error: invalid \"mark\" register \"%s\" for "
            "\"autoconf\"\n",
            mark_name);
        goto done;
      }

      (*autoconf)->mark = &marks[mark_name[0] - 'a'];
    } else if (!strcmp(key, "position")) {
      if (!parse_position(
              cur, &(*autoconf)->position.x, &(*autoconf)->position.y)) {
        fprintf(stderr,
            "configuration error: failed to parse \"autoconf\" \"position\"\n");
        goto done;
      }
    } else if (!strcmp(key, "focus")) {
      bool focus;

      if (!ucl_object_toboolean_safe(cur, &focus)) {
        fprintf(stderr,
            "configuration error: expected boolean for \"autoconf\" "
            "\"focus\"\n");
        goto done;
      }

      (*autoconf)->focus = focus;
    } else {
      fprintf(
          stderr, "configuration error: unkown \"autoconf\" key \"%s\"\n", key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_autoconfs(
    struct hikari_configuration *configuration, const ucl_object_t *obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    struct hikari_view_autoconf *autoconf =
        hikari_malloc(sizeof(struct hikari_view_autoconf));

    wl_list_insert(&hikari_configuration.autoconfs, &autoconf->link);

    const char *key = ucl_object_key(cur);
    size_t keylen = strlen(key);

    autoconf->app_id = hikari_malloc(keylen + 1);
    strcpy(autoconf->app_id, key);

    if (!parse_autoconf(cur, &autoconf)) {
      fprintf(stderr,
          "configuration error: failed to parse \"autoconf\" \"%s\"\n",
          key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

struct keycode_matcher_state {
  xkb_keysym_t keysym;
  uint32_t *keycode;
  struct xkb_state *state;
};

static void
match_keycode(struct xkb_keymap *keymap, xkb_keycode_t key, void *data)
{
  struct keycode_matcher_state *matcher_state = data;
  xkb_keysym_t keysym = xkb_state_key_get_one_sym(matcher_state->state, key);

  if (keysym != XKB_KEY_NoSymbol && keysym == matcher_state->keysym &&
      *(matcher_state->keycode) == 0) {
    *(matcher_state->keycode) = key - 8;
  }
}

static bool
parse_key(struct xkb_state *state,
    const char *str,
    uint32_t *keycode,
    uint32_t *modifiers)
{
  uint8_t mods = 0;
  const char *remaining;
  if (!parse_modifier_mask(str, &mods, &remaining)) {
    return false;
  }

  if (*remaining == '-') {
    errno = 0;
    const long value = strtol(remaining + 1, NULL, 10);
    if (errno != 0 || value < 0 || value > UINT32_MAX) {
      fprintf(stderr,
          "configuration error: failed to parse keycode \"%s\"\n",
          remaining + 1);
      return false;
    }
    *keycode = (uint32_t)value;
  } else {
    const xkb_keysym_t keysym =
        xkb_keysym_from_name(remaining + 1, XKB_KEYSYM_CASE_INSENSITIVE);
    if (keysym == XKB_KEY_NoSymbol) {
      fprintf(stderr,
          "configuration error: unknown key symbol \"%s\"\n",
          remaining + 1);
      return false;
    }

    *keycode = 0;
    struct keycode_matcher_state matcher_state = { keysym, keycode, state };
    xkb_keymap_key_for_each(
        xkb_state_get_keymap(state), match_keycode, &matcher_state);
  }

  *modifiers = mods;
  return true;
}

static bool
parse_pointer(struct xkb_state *ignored,
    const char *str,
    uint32_t *keycode,
    uint32_t *modifiers)
{
  uint8_t mods = 0;
  const char *remaining;
  if (!parse_modifier_mask(str, &mods, &remaining)) {
    return false;
  }

  if (*remaining == '-') {
    errno = 0;
    const long value = strtol(remaining + 1, NULL, 10);
    if (errno != 0 || value < 0 || value > UINT32_MAX) {
      fprintf(stderr,
          "configuration error: failed to parse mouse binding \"%s\"\n",
          remaining + 1);
      return false;
    }
    *keycode = (uint32_t)value;
  } else {
    ++remaining;
    if (!strcasecmp(remaining, "left")) {
      *keycode = BTN_LEFT;
    } else if (!strcasecmp(remaining, "right")) {
      *keycode = BTN_RIGHT;
    } else if (!strcasecmp(remaining, "middle")) {
      *keycode = BTN_MIDDLE;
    } else if (!strcasecmp(remaining, "side")) {
      *keycode = BTN_SIDE;
    } else if (!strcasecmp(remaining, "extra")) {
      *keycode = BTN_EXTRA;
    } else if (!strcasecmp(remaining, "forward")) {
      *keycode = BTN_FORWARD;
    } else if (!strcasecmp(remaining, "back")) {
      *keycode = BTN_BACK;
    } else if (!strcasecmp(remaining, "task")) {
      *keycode = BTN_TASK;
    } else {
      fprintf(stderr,
          "configuration error: unknown mouse button \"%s\"\n",
          remaining);
      return false;
    }
  }
  *modifiers = mods;
  return true;
}

static bool
parse_action(
    const char *action_name, const ucl_object_t *actions, char **command)
{
  const ucl_object_t *action_obj = ucl_object_lookup(actions, action_name + 7);

  if (action_obj != NULL) {
    *command = copy_in_config_string(action_obj);
    return true;
  } else {
    fprintf(
        stderr, "configuration error: unknown action \"%s\"\n", action_name);
    return false;
  }
}

static bool
parse_layout(const char *layout_name,
    const ucl_object_t *layouts,
    struct hikari_split **layout)
{
  struct hikari_split *ret = NULL;
  bool success = false;
  const ucl_object_t *layout_obj = ucl_object_lookup(layouts, layout_name + 7);

  if (layout_obj == NULL) {
    fprintf(
        stderr, "configuration error: unknown layout \"%s\"\n", layout_name);
  }

  if (!parse_split(layout_obj, &ret)) {
    fprintf(stderr,
        "configuration error: failed to parse layout \"%s\"\n",
        layout_name);
    goto done;
  }

  success = true;

done:
  *layout = ret;

  return success;
}

static void
cleanup_execute(void *arg)
{
  char *command = arg;
  hikari_free(command);
}

static void
cleanup_layout(void *arg)
{
  struct hikari_split *split = arg;
  hikari_split_fini(split);
}

#ifndef NDEBUG
static void
toggle_damage_tracking(void *arg)
{
  hikari_server.track_damage = !hikari_server.track_damage;

  struct hikari_output *output = NULL;
  wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
    hikari_output_damage_whole(output);
  }
}
#endif

static bool
parse_binding(const ucl_object_t *obj,
    const ucl_object_t *actions,
    const ucl_object_t *layouts,
    void (**action)(void *),
    void (**cleanup)(void *),
    void **arg)
{
  const char *str;
  bool success = false;

  if (!ucl_object_tostring_safe(obj, &str)) {
    fprintf(
        stderr, "configuration error: expected string for binding action\n");
    goto done;
  }

  if (!strcmp(str, "quit")) {
    *action = hikari_server_terminate;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "lock")) {
    *action = hikari_server_lock;
    *cleanup = NULL;
    *arg = NULL;
#ifndef NDEBUG
  } else if (!strcmp(str, "debug-damage")) {
    *action = toggle_damage_tracking;
    *cleanup = NULL;
    *arg = NULL;
#endif

  } else if (!strcmp(str, "view-move-up")) {
    *action = hikari_server_move_view_up;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-move-down")) {
    *action = hikari_server_move_view_down;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-move-left")) {
    *action = hikari_server_move_view_left;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-move-right")) {
    *action = hikari_server_move_view_right;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "view-snap-up")) {
    *action = hikari_server_snap_view_up;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-snap-down")) {
    *action = hikari_server_snap_view_down;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-snap-left")) {
    *action = hikari_server_snap_view_left;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-snap-right")) {
    *action = hikari_server_snap_view_right;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "view-toggle-maximize-vertical")) {
    *action = hikari_server_toggle_view_vertical_maximize;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-maximize-horizontal")) {
    *action = hikari_server_toggle_view_horizontal_maximize;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-maximize-full")) {
    *action = hikari_server_toggle_view_full_maximize;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-floating")) {
    *action = hikari_server_toggle_view_floating;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-iconified")) {
    *action = hikari_server_toggle_view_iconified;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "view-raise")) {
    *action = hikari_server_raise_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-lower")) {
    *action = hikari_server_lower_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-hide")) {
    *action = hikari_server_hide_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-only")) {
    *action = hikari_server_only_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-quit")) {
    *action = hikari_server_quit_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-reset-geometry")) {
    *action = hikari_server_reset_view_geometry;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "view-pin-to-sheet-0")) {
    *action = hikari_server_pin_view_to_sheet_0;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-1")) {
    *action = hikari_server_pin_view_to_sheet_1;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-2")) {
    *action = hikari_server_pin_view_to_sheet_2;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-3")) {
    *action = hikari_server_pin_view_to_sheet_3;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-4")) {
    *action = hikari_server_pin_view_to_sheet_4;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-5")) {
    *action = hikari_server_pin_view_to_sheet_5;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-6")) {
    *action = hikari_server_pin_view_to_sheet_6;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-7")) {
    *action = hikari_server_pin_view_to_sheet_7;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-8")) {
    *action = hikari_server_pin_view_to_sheet_8;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-9")) {
    *action = hikari_server_pin_view_to_sheet_9;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-alternate")) {
    *action = hikari_server_pin_view_to_sheet_alternate;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-current")) {
    *action = hikari_server_pin_view_to_sheet_current;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-next")) {
    *action = hikari_server_pin_view_to_sheet_next;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-prev")) {
    *action = hikari_server_pin_view_to_sheet_prev;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "view-decrease-size-up")) {
    *action = hikari_server_decrease_view_size_up;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-down")) {
    *action = hikari_server_increase_view_size_down;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-decrease-size-left")) {
    *action = hikari_server_decrease_view_size_left;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-right")) {
    *action = hikari_server_increase_view_size_right;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "sheet-switch-to-0")) {
    *action = hikari_server_display_sheet_0;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-1")) {
    *action = hikari_server_display_sheet_1;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-2")) {
    *action = hikari_server_display_sheet_2;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-3")) {
    *action = hikari_server_display_sheet_3;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-4")) {
    *action = hikari_server_display_sheet_4;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-5")) {
    *action = hikari_server_display_sheet_5;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-6")) {
    *action = hikari_server_display_sheet_6;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-7")) {
    *action = hikari_server_display_sheet_7;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-8")) {
    *action = hikari_server_display_sheet_8;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-9")) {
    *action = hikari_server_display_sheet_9;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-alternate")) {
    *action = hikari_server_display_sheet_alternate;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-current")) {
    *action = hikari_server_display_sheet_current;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-next")) {
    *action = hikari_server_display_sheet_next;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-prev")) {
    *action = hikari_server_display_sheet_prev;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-next-inhabited")) {
    *action = hikari_server_switch_to_next_inhabited_sheet;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-prev-inhabited")) {
    *action = hikari_server_switch_to_prev_inhabited_sheet;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "sheet-show-iconified")) {
    *action = hikari_server_show_iconified_sheet_views;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-show-all")) {
    *action = hikari_server_show_all_sheet_views;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-view-next")) {
    *action = hikari_server_cycle_next_sheet_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-view-prev")) {
    *action = hikari_server_cycle_prev_sheet_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-reset-layout")) {
    *action = hikari_server_reset_sheet_layout;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-next")) {
    *action = hikari_server_cycle_next_layout_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-prev")) {
    *action = hikari_server_cycle_prev_layout_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-first")) {
    *action = hikari_server_cycle_first_layout_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-last")) {
    *action = hikari_server_cycle_last_layout_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-exchange-layout-view-next")) {
    *action = hikari_server_exchange_next_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-exchange-layout-view-prev")) {
    *action = hikari_server_exchange_prev_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-exchange-layout-view-main")) {
    *action = hikari_server_exchange_main_layout_view;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "mode-enter-execute")) {
    *action = hikari_server_enter_exec_select_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-group-assign")) {
    *action = hikari_server_enter_group_assign_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-keyboard-grab")) {
    *action = hikari_server_enter_keyboard_grab_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-assign")) {
    *action = hikari_server_enter_mark_assign_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-select")) {
    *action = hikari_server_enter_mark_select_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-switch-select")) {
    *action = hikari_server_enter_mark_select_switch_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-move")) {
    *action = hikari_server_enter_move_mode;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-resize")) {
    *action = hikari_server_enter_resize_mode;
    *cleanup = NULL;
    *arg = NULL;

  } else if (!strcmp(str, "group-only")) {
    *action = hikari_server_only_group;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-hide")) {
    *action = hikari_server_hide_group;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-raise")) {
    *action = hikari_server_raise_group;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-lower")) {
    *action = hikari_server_lower_group;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-next")) {
    *action = hikari_server_cycle_next_group;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-prev")) {
    *action = hikari_server_cycle_prev_group;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-next")) {
    *action = hikari_server_cycle_next_group_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-prev")) {
    *action = hikari_server_cycle_prev_group_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-first")) {
    *action = hikari_server_cycle_first_group_view;
    *cleanup = NULL;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-last")) {
    *action = hikari_server_cycle_last_group_view;
    *cleanup = NULL;
    *arg = NULL;

  } else {
    if (!strncmp(str, "action-", 7)) {
      char *command = NULL;
      if (!parse_action(str, actions, &command)) {
        goto done;
      } else {
        *action = hikari_server_execute_command;
        *cleanup = cleanup_execute;
        *arg = command;
      }
    } else if (!strncmp(str, "layout-", 7)) {
      struct hikari_split *layout = NULL;
      if (!parse_layout(str, layouts, &layout)) {
        goto done;
      } else {
        *action = hikari_server_layout_sheet;
        *cleanup = cleanup_layout;
        *arg = layout;
      }
    } else {
      fprintf(stderr, "configuration error: unknown command \"%s\"\n", str);
      goto done;
    }
  }

  success = true;

done:
  return success;
}

static bool
parse_input_bindings(bool (*binding_parser)(struct xkb_state *xkb_state,
                         const char *str,
                         uint32_t *keysym,
                         uint32_t *modifiers),
    struct xkb_state *xkb_state,

    struct hikari_configuration *configuration,
    const ucl_object_t *actions,
    const ucl_object_t *layouts,
    uint8_t *nbindings,
    struct hikari_keybinding **bindings,
    const ucl_object_t *bindings_keysym)
{
  int n[256] = { 0 };
  uint8_t mask = 0;
  const ucl_object_t *cur;
  bool success = false;

  ucl_object_iter_t it = ucl_object_iterate_new(bindings_keysym);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!parse_modifier_mask(key, &mask, NULL)) {
      goto done;
    }
    n[mask]++;
  }
  ucl_object_iterate_free(it);

  for (int i = 0; i < 256; i++) {
    if (n[i] > 0) {
      nbindings[i] = n[i];
      bindings[i] = hikari_calloc(n[i], sizeof(struct hikari_keybinding));
    } else {
      bindings[i] = NULL;
    }

    n[i] = 0;
  }

  it = ucl_object_iterate_new(bindings_keysym);
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!parse_modifier_mask(key, &mask, NULL)) {
      goto done;
    }

    struct hikari_keybinding *binding = &bindings[mask][n[mask]];

    if (!binding_parser(
            xkb_state, key, &binding->keycode, &binding->modifiers) ||
        !parse_binding(cur,
            actions,
            layouts,
            &binding->action,
            &binding->cleanup,
            &binding->arg)) {
      goto done;
    }
    n[mask]++;
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_bindings(struct hikari_configuration *configuration,
    const ucl_object_t *actions,
    const ucl_object_t *layouts,
    const ucl_object_t *bindings_obj)
{
  bool success = false;
  const ucl_object_t *cur;

  struct xkb_keymap *keymap = hikari_load_keymap();
  struct xkb_state *state = xkb_state_new(keymap);
  xkb_keymap_unref(keymap);

  ucl_object_iter_t it = ucl_object_iterate_new(bindings_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "keyboard")) {
      if (!parse_input_bindings(&parse_key,
              state,
              configuration,
              actions,
              layouts,
              configuration->normal_mode.nkeybindings,
              configuration->normal_mode.keybindings,
              cur)) {
        goto done;
      }
    } else if (!strcmp(key, "mouse")) {
      if (!parse_input_bindings(&parse_pointer,
              state,
              configuration,
              actions,
              layouts,
              configuration->normal_mode.nmousebindings,
              configuration->normal_mode.mousebindings,
              cur)) {
        goto done;
      }
    } else {
      fprintf(stderr,
          "configuration error: unexpected \"bindings\" section \"%s\"\n",
          key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  xkb_state_unref(state);
  return success;
}

static bool
parse_pointer_config(struct hikari_configuration *configuration,
    const ucl_object_t *pointer_config_obj)
{
  bool success = false;
  const char *pointer_name = ucl_object_key(pointer_config_obj);
  size_t keylen = strlen(pointer_name);
  const ucl_object_t *cur;

  struct hikari_pointer_config *pointer_config =
      hikari_malloc(sizeof(struct hikari_pointer_config));

  pointer_config->name = hikari_malloc(keylen + 1);
  strcpy(pointer_config->name, pointer_name);
  pointer_config->accel = 0;
  pointer_config->scroll_method = LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
  pointer_config->scroll_button = 0;

  wl_list_insert(&configuration->pointer_configs, &pointer_config->link);

  ucl_object_iter_t it = ucl_object_iterate_new(pointer_config_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "accel")) {
      double accel;
      if (!ucl_object_todouble_safe(cur, &accel)) {
        fprintf(stderr,
            "configuration error: expected float for \"%s\" \"accel\"\n",
            pointer_name);
        goto done;
      }

      pointer_config->accel = accel;
    } else if (!strcmp(key, "scroll-method")) {
      const char *scroll_method;
      if (!ucl_object_tostring_safe(cur, &scroll_method)) {
        fprintf(stderr,
            "configuration error: expected string \"%s\" for "
            "\"scroll-method\"\n",
            pointer_name);
        goto done;
      }

      if (!strcmp(scroll_method, "on-button-down")) {
        pointer_config->scroll_method = LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;
      } else {
        fprintf(stderr,
            "configuration error: unkown \"scroll-method\" \"%s\" for \"%s\"\n",
            scroll_method,
            pointer_name);
        goto done;
      }
    } else if (!strcmp(key, "scroll-button")) {
      int64_t scroll_button;
      if (!ucl_object_toint_safe(cur, &scroll_button)) {
        fprintf(stderr,
            "configuration error: expected integer for \"%s\" "
            "\"scroll-button\"\n",
            pointer_name);
        goto done;
      }

      pointer_config->scroll_button = scroll_button;
    } else {
      fprintf(stderr,
          "configuration error: unknown \"pointer\" configuration key \"%s\" "
          "for "
          "\"%s\"\n",
          key,
          pointer_name);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_pointers(struct hikari_configuration *configuration,
    const ucl_object_t *pointers_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(pointers_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    if (!parse_pointer_config(configuration, cur)) {
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_inputs(
    struct hikari_configuration *configuration, const ucl_object_t *inputs_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(inputs_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "pointers")) {
      if (!parse_pointers(configuration, cur)) {
        goto done;
      }
    } else {
      fprintf(stderr,
          "configuration error: unknown \"inputs\" configuration key \"%s\"",
          key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_background(struct hikari_configuration *configuration,
    const ucl_object_t *background_obj)
{
  const char *key = ucl_object_key(background_obj);
  char *background_path = copy_in_config_string(background_obj);
  struct hikari_background *background =
      hikari_malloc(sizeof(struct hikari_background));

  wl_list_insert(&configuration->backgrounds, &background->link);

  if (background_path != NULL) {
    hikari_background_init(background, key, background_path);
    return true;
  } else {
    fprintf(stderr,
        "configuration error: expected string for background image path\n");
    return false;
  }
}

static bool
parse_backgrounds(struct hikari_configuration *configuration,
    const ucl_object_t *backgrounds_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(backgrounds_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    if (!parse_background(configuration, cur)) {
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_border(
    struct hikari_configuration *configuration, const ucl_object_t *border_obj)
{
  int64_t border;

  if (!ucl_object_toint_safe(border_obj, &border)) {
    fprintf(stderr, "configuration error: expected integer for \"border\"\n");
    return false;
  }

  configuration->border = border;

  return true;
}

static bool
parse_gap(
    struct hikari_configuration *configuration, const ucl_object_t *gap_obj)
{
  int64_t gap;

  if (!ucl_object_toint_safe(gap_obj, &gap)) {
    fprintf(stderr, "configuration error: expected integer for \"gap\"\n");
    return false;
  }

  configuration->gap = gap;

  return true;
}

static bool
parse_font(
    struct hikari_configuration *configuration, const ucl_object_t *font_obj)
{
  const char *font;

  if (!ucl_object_tostring_safe(font_obj, &font)) {
    fprintf(stderr, "configuration error: expected string for \"font\"\n");
    return false;
  }

  hikari_font_init(&configuration->font, font);

  return true;
}

static char *
get_config_path(void)
{
  char *config_home = getenv("HOME");
  char *path = "/.config/hikari/hikari.conf";
  size_t len = strlen(config_home) + strlen(path);

  char *ret = malloc(len + 1);

  memset(ret, 0, len + 1);

  strcat(ret, config_home);
  strcat(ret, path);

  return ret;
}

bool
hikari_configuration_load(struct hikari_configuration *configuration)
{
  struct ucl_parser *parser = ucl_parser_new(0);
  char *config_path = get_config_path();
  bool success = false;
  const ucl_object_t *cur;

  ucl_parser_add_file(parser, config_path);
  ucl_object_t *configuration_obj = ucl_parser_get_object(parser);

  ucl_object_iter_t it = ucl_object_iterate_new(configuration_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "colorscheme")) {
      if (!parse_colorscheme(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "autoconf")) {
      if (!parse_autoconfs(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "execute")) {
      if (!parse_execute(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "bindings")) {
      const ucl_object_t *actions =
          ucl_object_lookup(configuration_obj, "actions");
      const ucl_object_t *layouts =
          ucl_object_lookup(configuration_obj, "layouts");

      if (!parse_bindings(configuration, actions, layouts, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "backgrounds")) {
      if (!parse_backgrounds(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "inputs")) {
      if (!parse_inputs(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "font")) {
      hikari_font_fini(&configuration->font);
      if (!parse_font(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "border")) {
      if (!parse_border(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "gap")) {
      if (!parse_gap(configuration, cur)) {
        goto done;
      }
    } else if (!!strcmp(key, "actions") && !!strcmp(key, "layouts")) {
      fprintf(stderr,
          "configuration error: unkown configuration section \"%s\"\n",
          key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);
  ucl_object_unref(configuration_obj);
  ucl_parser_free(parser);

  free(config_path);

  return success;
}

void
hikari_configuration_init(struct hikari_configuration *configuration)
{
  wl_list_init(&configuration->autoconfs);
  wl_list_init(&configuration->backgrounds);
  wl_list_init(&configuration->pointer_configs);

  hikari_color_convert(configuration->clear, 0x282C34);
  hikari_color_convert(configuration->foreground, 0x000000);
  hikari_color_convert(configuration->indicator_selected, 0xE6DB74);
  hikari_color_convert(configuration->indicator_grouped, 0xFD971F);
  hikari_color_convert(configuration->indicator_first, 0xB8E673);
  hikari_color_convert(configuration->indicator_conflict, 0xEF5939);
  hikari_color_convert(configuration->indicator_insert, 0x66D9EF);
  hikari_color_convert(configuration->border_active, 0xFFFFFF);
  hikari_color_convert(configuration->border_inactive, 0x465457);

  hikari_font_init(&configuration->font, "DejaVu Sans Mono 10");

  configuration->border = 1;
  configuration->gap = 5;
}

static void
cleanup_bindings(struct hikari_keybinding **bindings, uint8_t *n)
{
  for (int i = 0; i < 256; i++) {
    struct hikari_keybinding *mod_bindings = bindings[i];
    uint8_t nbindings = n[i];

    for (uint8_t j = 0; j < nbindings; j++) {
      struct hikari_keybinding *binding = &mod_bindings[j];
      if (binding != NULL) {
        hikari_keybinding_fini(binding);
      }
    }
    hikari_free(mod_bindings);
  }
}

void
hikari_configuration_fini(struct hikari_configuration *configuration)
{
  cleanup_bindings(configuration->normal_mode.keybindings,
      configuration->normal_mode.nkeybindings);

  cleanup_bindings(configuration->normal_mode.mousebindings,
      configuration->normal_mode.nmousebindings);

  struct hikari_view_autoconf *autoconf, *autoconf_temp;
  wl_list_for_each_safe (
      autoconf, autoconf_temp, &configuration->autoconfs, link) {
    wl_list_remove(&autoconf->link);

    hikari_view_autoconf_fini(autoconf);
    hikari_free(autoconf);
  }

  struct hikari_background *background, *background_temp;
  wl_list_for_each_safe (
      background, background_temp, &configuration->backgrounds, link) {
    wl_list_remove(&background->link);

    hikari_background_fini(background);
    hikari_free(background);
  }

  struct hikari_pointer_config *pointer_config, *pointer_config_temp;
  wl_list_for_each_safe (pointer_config,
      pointer_config_temp,
      &configuration->pointer_configs,
      link) {
    wl_list_remove(&pointer_config->link);

    hikari_pointer_config_fini(pointer_config);
    hikari_free(pointer_config);
  }
}

char *
hikari_configuration_resolve_background(
    struct hikari_configuration *configuration, const char *output_name)
{
  struct hikari_background *background;
  wl_list_for_each (background, &hikari_configuration.backgrounds, link) {
    if (!strcmp(background->output_name, output_name)) {
      return background->path;
    }
  }

  return NULL;
}

struct hikari_pointer_config *
hikari_configuration_resolve_pointer_config(
    struct hikari_configuration *configuration, const char *pointer_name)
{
  struct hikari_pointer_config *pointer_config;
  wl_list_for_each (
      pointer_config, &hikari_configuration.pointer_configs, link) {
    if (!strcmp(pointer_config->name, pointer_name)) {
      return pointer_config;
    }
  }

  return NULL;
}
