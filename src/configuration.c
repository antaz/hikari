#include <hikari/configuration.h>

#include <ctype.h>
#include <errno.h>

#include <ucl.h>

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_cursor.h>

#include <hikari/action_config.h>
#include <hikari/color.h>
#include <hikari/command.h>
#include <hikari/exec.h>
#include <hikari/keybinding.h>
#include <hikari/keyboard.h>
#include <hikari/layout.h>
#include <hikari/layout_config.h>
#include <hikari/mark.h>
#include <hikari/memory.h>
#include <hikari/output.h>
#include <hikari/output_config.h>
#include <hikari/pointer.h>
#include <hikari/pointer_config.h>
#include <hikari/server.h>
#include <hikari/sheet.h>
#include <hikari/split.h>
#include <hikari/tile.h>
#include <hikari/view.h>
#include <hikari/view_autoconf.h>
#include <hikari/workspace.h>

struct hikari_configuration *hikari_configuration = NULL;

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
parse_layout_func(const char *layout_func_name,
    layout_func_t *layout_func,
    int64_t *views,
    bool *explicit_nr_of_views)
{
  if (!strcmp(layout_func_name, "queue")) {
    *layout_func = hikari_sheet_queue_layout;
  } else if (!strcmp(layout_func_name, "stack")) {
    *layout_func = hikari_sheet_stack_layout;
  } else if (!strcmp(layout_func_name, "full")) {
    *layout_func = hikari_sheet_full_layout;
  } else if (!strcmp(layout_func_name, "grid")) {
    *layout_func = hikari_sheet_grid_layout;
  } else if (!strcmp(layout_func_name, "single")) {
    *views = 1;
    *explicit_nr_of_views = true;
    *layout_func = hikari_sheet_single_layout;
  } else if (!strcmp(layout_func_name, "empty")) {
    *views = 0;
    *explicit_nr_of_views = true;
    *layout_func = hikari_sheet_empty_layout;
  } else {
    fprintf(stderr,
        "configuration error: unknown container \"layout\" \"%s\"\n",
        layout_func_name);
    return false;
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
  int64_t views = 256;
  bool explicit_nr_of_views = false;
  bool override_nr_of_views = false;
  const char *layout_func_name;
  const ucl_object_t *cur;

  ucl_object_iter_t it = ucl_object_iterate_new(container_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "layout")) {
      if (!ucl_object_tostring_safe(cur, &layout_func_name)) {
        fprintf(stderr,
            "configuration error: expected string for container \"layout\"\n");
        goto done;
      }

      if (!parse_layout_func(
              layout_func_name, &layout_func, &views, &explicit_nr_of_views)) {
        goto done;
      }
    } else if (!strcmp(key, "views")) {
      override_nr_of_views = true;
      if (explicit_nr_of_views) {
        fprintf(stderr,
            "configuration error: cannot set \"views\" for \"layout\" \"%s\" "
            "container\n",
            layout_func_name);
        goto done;
      }

      if (!ucl_object_toint_safe(cur, &views)) {
        fprintf(stderr,
            "configuration error: expected integer for container \"views\"\n");
        goto done;
      }

      if (views < 2 || views > 256) {
        fprintf(stderr,
            "configuration error: expected integer between 2 and 256 for "
            "container \"views\"\n");
        goto done;
      }
    } else {
      fprintf(
          stderr, "configuration error: unknown container key \"%s\"\n", key);
      goto done;
    }
  }

  if (layout_func == NULL) {
    fprintf(stderr, "configuration error: container expects \"layout\"\n");
    goto done;
  }

  if (override_nr_of_views && explicit_nr_of_views) {
    fprintf(stderr,
        "configuration error: cannot set \"views\" for \"layout\" \"%s\" "
        "container\n",
        layout_func_name);
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
split_is_container(const ucl_object_t *top,
    const ucl_object_t *bottom,
    const ucl_object_t *left,
    const ucl_object_t *right,
    const ucl_object_t *layout)
{
  return layout != NULL && left == NULL && right == NULL && top == NULL &&
         bottom == NULL;
}

static bool
split_is_vertical(const ucl_object_t *top,
    const ucl_object_t *bottom,
    const ucl_object_t *left,
    const ucl_object_t *right,
    const ucl_object_t *layout)
{
  return left != NULL && right != NULL && top == NULL && bottom == NULL &&
         layout == NULL;
}

static bool
split_is_horizontal(const ucl_object_t *top,
    const ucl_object_t *bottom,
    const ucl_object_t *left,
    const ucl_object_t *right,
    const ucl_object_t *layout)
{
  return top != NULL && bottom != NULL && left == NULL && right == NULL &&
         layout == NULL;
}

static bool
parse_vertical(const ucl_object_t *, struct hikari_split **);

static bool
parse_horizontal(const ucl_object_t *, struct hikari_split **);

static bool
parse_split(const ucl_object_t *split_obj, struct hikari_split **split)
{
  bool success = false;
  struct hikari_split *ret = NULL;
  ucl_type_t type = ucl_object_type(split_obj);

  if (type == UCL_STRING) {
    const char *layout_func_name;
    layout_func_t layout_func;
    int64_t views = 256;
    bool explicit_nr_of_views = false;
    if (!ucl_object_tostring_safe(split_obj, &layout_func_name)) {
      fprintf(stderr,
          "configuration error: expected string for container layout\n");
      goto done;
    }

    if (!parse_layout_func(
            layout_func_name, &layout_func, &views, &explicit_nr_of_views)) {
      goto done;
    }

    ret = hikari_malloc(sizeof(struct hikari_container));
    hikari_container_init((struct hikari_container *)ret, views, layout_func);

  } else if (type == UCL_OBJECT) {
    const ucl_object_t *left = ucl_object_lookup(split_obj, "left");
    const ucl_object_t *right = ucl_object_lookup(split_obj, "right");
    const ucl_object_t *top = ucl_object_lookup(split_obj, "top");
    const ucl_object_t *bottom = ucl_object_lookup(split_obj, "bottom");
    const ucl_object_t *layout = ucl_object_lookup(split_obj, "layout");

    if (split_is_vertical(top, bottom, left, right, layout)) {
      if (!parse_vertical(split_obj, &ret)) {
        fprintf(
            stderr, "configuration error: failed to parse vertical split\n");
        goto done;
      }
    } else if (split_is_horizontal(top, bottom, left, right, layout)) {
      if (!parse_horizontal(split_obj, &ret)) {
        fprintf(
            stderr, "configuration error: failed to parse horizontal split\n");
        goto done;
      }
    } else if (split_is_container(top, bottom, left, right, layout)) {
      if (!parse_container(split_obj, &ret)) {
        fprintf(stderr, "configuration error: failed to parse container\n");
        goto done;
      }
    } else {
      fprintf(
          stderr, "configuration error: failed to determine layout element\n");
      goto done;
    }
  } else {
    fprintf(stderr,
        "configuration error: expected string or object for layout element\n");
    goto done;
  }

  success = true;

done:
  *split = ret;

  return success;
}

static bool
parse_scale(const ucl_object_t *scale_obj, double *scale)
{
  bool success = false;
  double ret = 0.5;
  if (scale_obj != NULL) {
    if (!ucl_object_todouble_safe(scale_obj, &ret)) {
      fprintf(stderr, "configuration error: expected float for \"scale\"\n");
      goto done;
    }

    if (ret < 0.1 || ret > 0.9) {
      fprintf(stderr,
          "configuration error: \"scale\" of \"%.2f\" is not between \"0.1\" "
          "and "
          "\"0.9\"\n",
          ret);
      goto done;
    }
  }

  success = true;

done:
  *scale = ret;

  return success;
}

#define PARSE_SPLIT(name, NAME, first, FIRST, second, SECOND)                  \
  static bool parse_##name(                                                    \
      const ucl_object_t *name##_obj, struct hikari_split **name)              \
  {                                                                            \
    bool success = false;                                                      \
    bool found_orientation = false;                                            \
    const ucl_object_t *cur;                                                   \
    struct hikari_##name##_split *ret = NULL;                                  \
    double scale = 0.5;                                                        \
    enum hikari_##name##_split_orientation orientation =                       \
        HIKARI_##NAME##_SPLIT_ORIENTATION_##FIRST;                             \
    struct hikari_split *first = NULL;                                         \
    struct hikari_split *second = NULL;                                        \
                                                                               \
    ucl_object_iter_t it = ucl_object_iterate_new(name##_obj);                 \
                                                                               \
    while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {               \
      const char *key = ucl_object_key(cur);                                   \
                                                                               \
      if (!strcmp(key, "scale")) {                                             \
        if (!parse_scale(cur, &scale)) {                                       \
          goto done;                                                           \
        }                                                                      \
      } else if (!strcmp(key, #first)) {                                       \
        if (!parse_split(cur, &first)) {                                       \
          fprintf(stderr,                                                      \
              "configuration error: invalid \"" #first "\" for \"" #name       \
              "\" split\n");                                                   \
          goto done;                                                           \
        }                                                                      \
                                                                               \
        if (!found_orientation) {                                              \
          orientation = HIKARI_##NAME##_SPLIT_ORIENTATION_##FIRST;             \
          found_orientation = true;                                            \
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
                                                                               \
        if (!found_orientation) {                                              \
          orientation = HIKARI_##NAME##_SPLIT_ORIENTATION_##SECOND;            \
          found_orientation = true;                                            \
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
    hikari_##name##_split_init(ret, scale, orientation, first, second);        \
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

PARSE_SPLIT(vertical, VERTICAL, left, LEFT, right, RIGHT);
PARSE_SPLIT(horizontal, HORIZONTAL, top, TOP, bottom, BOTTOM);
#undef PARSE_SPLIT

static struct hikari_view_autoconf *
resolve_autoconf(struct hikari_configuration *configuration, const char *app_id)
{
  if (app_id != NULL) {
    struct hikari_view_autoconf *autoconf;
    wl_list_for_each (autoconf, &configuration->autoconfs, link) {
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
      resolve_autoconf(configuration, app_id);

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
      execute = &configuration->execs[nr];
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
parse_autoconf(struct hikari_configuration *configuration,
    const ucl_object_t *autoconf_obj,
    struct hikari_view_autoconf **autoconf)
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
        hikari_free(group_name);
        fprintf(stderr,
            "configuration error: expected non-empty string for \"autoconf\" "
            "\"group\"\n");
        goto done;
      } else if (isdigit(group_name[0])) {
        fprintf(stderr,
            "configuration error: \"autoconf\" "
            "\"group\" \"%s\" starts with digit\n",
            group_name);
        hikari_free(group_name);
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

      (*autoconf)->mark = &hikari_marks[mark_name[0] - 'a'];
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

    wl_list_insert(&configuration->autoconfs, &autoconf->link);

    const char *key = ucl_object_key(cur);
    size_t keylen = strlen(key);

    autoconf->app_id = hikari_malloc(keylen + 1);
    strcpy(autoconf->app_id, key);

    if (!parse_autoconf(configuration, cur, &autoconf)) {
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
parse_mouse_button(const char *str, uint32_t *keycode)
{
  if (!strcasecmp(str, "left")) {
    *keycode = BTN_LEFT;
  } else if (!strcasecmp(str, "right")) {
    *keycode = BTN_RIGHT;
  } else if (!strcasecmp(str, "middle")) {
    *keycode = BTN_MIDDLE;
  } else if (!strcasecmp(str, "side")) {
    *keycode = BTN_SIDE;
  } else if (!strcasecmp(str, "extra")) {
    *keycode = BTN_EXTRA;
  } else if (!strcasecmp(str, "forward")) {
    *keycode = BTN_FORWARD;
  } else if (!strcasecmp(str, "back")) {
    *keycode = BTN_BACK;
  } else if (!strcasecmp(str, "task")) {
    *keycode = BTN_TASK;
  } else {
    fprintf(stderr, "configuration error: unknown mouse button \"%s\"\n", str);
    return false;
  }

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
    if (!parse_mouse_button(remaining, keycode)) {
      return false;
    }
  }
  *modifiers = mods;
  return true;
}

static bool
parse_action(const char *action_name,
    const ucl_object_t *action_obj,
    const char **command)
{
  if (!ucl_object_tostring_safe(action_obj, command)) {
    fprintf(stderr,
        "configuration error: expected string for \"action\" command\n");
    return false;
  }

  return true;
}

static bool
parse_actions(
    struct hikari_configuration *configuration, const ucl_object_t *actions_obj)
{
  bool success = false;
  struct hikari_action_config *action_config;

  const ucl_object_t *cur;
  ucl_object_iter_t it = ucl_object_iterate_new(actions_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);
    const char *command;

    if (!parse_action(key, cur, &command)) {
      goto done;
    }

    action_config = hikari_malloc(sizeof(struct hikari_action_config));
    hikari_action_config_init(action_config, key, command);

    wl_list_insert(&configuration->action_configs, &action_config->link);
  }

  success = true;

done:
  return success;
}

static bool
parse_layout(const char *layout_name,
    const ucl_object_t *layout_obj,
    struct hikari_split **split)
{
  struct hikari_split *ret = NULL;

  if (!parse_split(layout_obj, &ret)) {
    fprintf(stderr,
        "configuration error: failed to parse layout \"%s\"\n",
        layout_name);
    return false;
  }

  *split = ret;

  return true;
}

static bool
parse_layouts(
    struct hikari_configuration *configuration, const ucl_object_t *layouts_obj)
{
  bool success = false;
  struct hikari_layout_config *layout_config;
  struct hikari_split *split;

  const ucl_object_t *cur;
  ucl_object_iter_t it = ucl_object_iterate_new(layouts_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!parse_layout(key, cur, &split)) {
      goto done;
    }

    layout_config = hikari_malloc(sizeof(struct hikari_layout_config));
    hikari_layout_config_init(layout_config, key, split);

    wl_list_insert(&configuration->layout_configs, &layout_config->link);
  }

  success = true;

done:
  return success;
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

struct hikari_split *
lookup_layout(
    struct hikari_configuration *configuration, const char *layout_name)
{
  struct hikari_layout_config *layout_config;
  wl_list_for_each (layout_config, &configuration->layout_configs, link) {
    if (!strcmp(layout_name, layout_config->layout_name)) {
      return layout_config->split;
    }
  }

  return NULL;
}

char *
lookup_action(
    struct hikari_configuration *configuration, const char *action_name)
{
  struct hikari_action_config *action_config;
  wl_list_for_each (action_config, &configuration->action_configs, link) {
    if (!strcmp(action_name, action_config->action_name)) {
      return action_config->command;
    }
  }

  return NULL;
}

static bool
parse_binding(struct hikari_configuration *configuration,
    const ucl_object_t *obj,
    void (**action)(void *),
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
    *arg = NULL;
  } else if (!strcmp(str, "lock")) {
    *action = hikari_server_lock;
    *arg = NULL;
  } else if (!strcmp(str, "reload")) {
    *action = hikari_server_reload;
    *arg = NULL;
#ifndef NDEBUG
  } else if (!strcmp(str, "debug-damage")) {
    *action = toggle_damage_tracking;
    *arg = NULL;
#endif

  } else if (!strcmp(str, "view-move-up")) {
    *action = hikari_server_move_view_up;
    *arg = NULL;
  } else if (!strcmp(str, "view-move-down")) {
    *action = hikari_server_move_view_down;
    *arg = NULL;
  } else if (!strcmp(str, "view-move-left")) {
    *action = hikari_server_move_view_left;
    *arg = NULL;
  } else if (!strcmp(str, "view-move-right")) {
    *action = hikari_server_move_view_right;
    *arg = NULL;

  } else if (!strcmp(str, "view-snap-up")) {
    *action = hikari_server_snap_view_up;
    *arg = NULL;
  } else if (!strcmp(str, "view-snap-down")) {
    *action = hikari_server_snap_view_down;
    *arg = NULL;
  } else if (!strcmp(str, "view-snap-left")) {
    *action = hikari_server_snap_view_left;
    *arg = NULL;
  } else if (!strcmp(str, "view-snap-right")) {
    *action = hikari_server_snap_view_right;
    *arg = NULL;

  } else if (!strcmp(str, "view-toggle-maximize-vertical")) {
    *action = hikari_server_toggle_view_vertical_maximize;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-maximize-horizontal")) {
    *action = hikari_server_toggle_view_horizontal_maximize;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-maximize-full")) {
    *action = hikari_server_toggle_view_full_maximize;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-floating")) {
    *action = hikari_server_toggle_view_floating;
    *arg = NULL;
  } else if (!strcmp(str, "view-toggle-iconified")) {
    *action = hikari_server_toggle_view_iconified;
    *arg = NULL;

  } else if (!strcmp(str, "view-raise")) {
    *action = hikari_server_raise_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-lower")) {
    *action = hikari_server_lower_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-hide")) {
    *action = hikari_server_hide_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-only")) {
    *action = hikari_server_only_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-quit")) {
    *action = hikari_server_quit_view;
    *arg = NULL;
  } else if (!strcmp(str, "view-reset-geometry")) {
    *action = hikari_server_reset_view_geometry;
    *arg = NULL;

  } else if (!strcmp(str, "view-pin-to-sheet-0")) {
    *action = hikari_server_pin_view_to_sheet_0;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-1")) {
    *action = hikari_server_pin_view_to_sheet_1;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-2")) {
    *action = hikari_server_pin_view_to_sheet_2;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-3")) {
    *action = hikari_server_pin_view_to_sheet_3;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-4")) {
    *action = hikari_server_pin_view_to_sheet_4;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-5")) {
    *action = hikari_server_pin_view_to_sheet_5;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-6")) {
    *action = hikari_server_pin_view_to_sheet_6;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-7")) {
    *action = hikari_server_pin_view_to_sheet_7;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-8")) {
    *action = hikari_server_pin_view_to_sheet_8;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-9")) {
    *action = hikari_server_pin_view_to_sheet_9;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-alternate")) {
    *action = hikari_server_pin_view_to_sheet_alternate;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-current")) {
    *action = hikari_server_pin_view_to_sheet_current;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-next")) {
    *action = hikari_server_pin_view_to_sheet_next;
    *arg = NULL;
  } else if (!strcmp(str, "view-pin-to-sheet-prev")) {
    *action = hikari_server_pin_view_to_sheet_prev;
    *arg = NULL;

  } else if (!strcmp(str, "view-decrease-size-up")) {
    *action = hikari_server_decrease_view_size_up;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-down")) {
    *action = hikari_server_increase_view_size_down;
    *arg = NULL;
  } else if (!strcmp(str, "view-decrease-size-left")) {
    *action = hikari_server_decrease_view_size_left;
    *arg = NULL;
  } else if (!strcmp(str, "view-increase-size-right")) {
    *action = hikari_server_increase_view_size_right;
    *arg = NULL;

  } else if (!strcmp(str, "sheet-switch-to-0")) {
    *action = hikari_server_display_sheet_0;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-1")) {
    *action = hikari_server_display_sheet_1;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-2")) {
    *action = hikari_server_display_sheet_2;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-3")) {
    *action = hikari_server_display_sheet_3;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-4")) {
    *action = hikari_server_display_sheet_4;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-5")) {
    *action = hikari_server_display_sheet_5;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-6")) {
    *action = hikari_server_display_sheet_6;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-7")) {
    *action = hikari_server_display_sheet_7;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-8")) {
    *action = hikari_server_display_sheet_8;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-9")) {
    *action = hikari_server_display_sheet_9;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-alternate")) {
    *action = hikari_server_display_sheet_alternate;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-current")) {
    *action = hikari_server_display_sheet_current;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-next")) {
    *action = hikari_server_display_sheet_next;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-prev")) {
    *action = hikari_server_display_sheet_prev;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-next-inhabited")) {
    *action = hikari_server_switch_to_next_inhabited_sheet;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-switch-to-prev-inhabited")) {
    *action = hikari_server_switch_to_prev_inhabited_sheet;
    *arg = NULL;

  } else if (!strcmp(str, "sheet-show-iconified")) {
    *action = hikari_server_show_iconified_sheet_views;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-show-all")) {
    *action = hikari_server_show_all_sheet_views;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-view-next")) {
    *action = hikari_server_cycle_next_sheet_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-view-prev")) {
    *action = hikari_server_cycle_prev_sheet_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-reset-layout")) {
    *action = hikari_server_reset_sheet_layout;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-next")) {
    *action = hikari_server_cycle_next_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-prev")) {
    *action = hikari_server_cycle_prev_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-first")) {
    *action = hikari_server_cycle_first_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-cycle-layout-view-last")) {
    *action = hikari_server_cycle_last_layout_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-exchange-layout-view-next")) {
    *action = hikari_server_exchange_next_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-exchange-layout-view-prev")) {
    *action = hikari_server_exchange_prev_view;
    *arg = NULL;
  } else if (!strcmp(str, "sheet-exchange-layout-view-main")) {
    *action = hikari_server_exchange_main_layout_view;
    *arg = NULL;

  } else if (!strcmp(str, "mode-enter-execute")) {
    *action = hikari_server_enter_exec_select_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-group-assign")) {
    *action = hikari_server_enter_group_assign_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-keyboard-grab")) {
    *action = hikari_server_enter_keyboard_grab_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-assign")) {
    *action = hikari_server_enter_mark_assign_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-select")) {
    *action = hikari_server_enter_mark_select_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-mark-switch-select")) {
    *action = hikari_server_enter_mark_select_switch_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-move")) {
    *action = hikari_server_enter_move_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-resize")) {
    *action = hikari_server_enter_resize_mode;
    *arg = NULL;
  } else if (!strcmp(str, "mode-enter-sheet-assign")) {
    *action = hikari_server_enter_sheet_assign_mode;
    *arg = NULL;

  } else if (!strcmp(str, "group-only")) {
    *action = hikari_server_only_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-hide")) {
    *action = hikari_server_hide_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-raise")) {
    *action = hikari_server_raise_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-lower")) {
    *action = hikari_server_lower_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-next")) {
    *action = hikari_server_cycle_next_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-prev")) {
    *action = hikari_server_cycle_prev_group;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-next")) {
    *action = hikari_server_cycle_next_group_view;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-prev")) {
    *action = hikari_server_cycle_prev_group_view;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-first")) {
    *action = hikari_server_cycle_first_group_view;
    *arg = NULL;
  } else if (!strcmp(str, "group-cycle-view-last")) {
    *action = hikari_server_cycle_last_group_view;
    *arg = NULL;

  } else if (!strcmp(str, "vt-switch-to-1")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)1;
  } else if (!strcmp(str, "vt-switch-to-2")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)2;
  } else if (!strcmp(str, "vt-switch-to-3")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)3;
  } else if (!strcmp(str, "vt-switch-to-4")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)4;
  } else if (!strcmp(str, "vt-switch-to-5")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)5;
  } else if (!strcmp(str, "vt-switch-to-6")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)6;
  } else if (!strcmp(str, "vt-switch-to-7")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)7;
  } else if (!strcmp(str, "vt-switch-to-8")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)8;
  } else if (!strcmp(str, "vt-switch-to-9")) {
    *action = hikari_server_session_change_vt;
    *arg = (void *)(intptr_t)9;

  } else {
    if (!strncmp(str, "action-", 7)) {
      char *command = NULL;
      const char *action_name = &str[7];
      if ((command = lookup_action(configuration, action_name)) == NULL) {
        fprintf(stderr,
            "configuration error: unknown action \"%s\"\n",
            action_name);
        goto done;
      } else {
        *action = hikari_server_execute_command;
        *arg = command;
      }
    } else if (!strncmp(str, "layout-", 7)) {
      const char *layout_name = &str[7];
      struct hikari_split *layout = NULL;
      if ((layout = lookup_layout(configuration, layout_name)) == NULL) {
        fprintf(stderr,
            "configuration error: unknown layout \"%s\"\n",
            layout_name);
        goto done;
      } else {
        *action = hikari_server_layout_sheet;
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
        !parse_binding(configuration, cur, &binding->action, &binding->arg)) {
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
      if (!parse_input_bindings(parse_key,
              state,
              configuration,
              actions,
              layouts,
              configuration->bindings.nkeybindings,
              configuration->bindings.keybindings,
              cur)) {
        goto done;
      }
    } else if (!strcmp(key, "mouse")) {
      if (!parse_input_bindings(parse_pointer,
              state,
              configuration,
              actions,
              layouts,
              configuration->bindings.nmousebindings,
              configuration->bindings.mousebindings,
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
      const char *scroll_button;
      uint32_t scroll_button_keycode;
      if (!ucl_object_tostring_safe(cur, &scroll_button)) {
        fprintf(stderr,
            "configuration error: expected string for \"scroll-button\"\n");
        goto done;
      }

      if (!parse_mouse_button(scroll_button, &scroll_button_keycode)) {
        fprintf(
            stderr, "configuration error: failed to parse \"scroll-button\"\n");
        goto done;
      }

      pointer_config->scroll_button = scroll_button_keycode;
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
parse_output_config(struct hikari_configuration *configuration,
    const ucl_object_t *output_config_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(output_config_obj);
  const char *output_name = ucl_object_key(output_config_obj);
  char *background = NULL;
  enum hikari_background_fit background_fit = HIKARI_BACKGROUND_STRETCH;

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "background")) {
      background = copy_in_config_string(cur);

      if (background == NULL) {
        fprintf(
            stderr, "configuration error: failed to parse \"background\"\n");
        goto done;
      }
    } else if (!strcmp(key, "background_fit")) {
      const char *fit_value;
      if (!ucl_object_tostring_safe(cur, &fit_value)) {
        fprintf(stderr,
            "configuration error: failed to parse \"background_fit\"\n");
        goto done;
      }

      if (!strcmp(fit_value, "center")) {
        background_fit = HIKARI_BACKGROUND_CENTER;
      } else if (!strcmp(fit_value, "stretch")) {
        background_fit = HIKARI_BACKGROUND_STRETCH;
      } else if (!strcmp(fit_value, "tile")) {
        background_fit = HIKARI_BACKGROUND_TILE;
      } else {
        fprintf(stderr,
            "configuration error: failed to parse \"background_fit\"\n");
        goto done;
      }
    } else {
      fprintf(stderr,
          "configuration error: unknown \"outputs\" configuration key \"%s\"\n",
          key);
    }
  }

  struct hikari_output_config *output_config =
      hikari_malloc(sizeof(struct hikari_output_config));
  hikari_output_config_init(
      output_config, output_name, background, background_fit);

  wl_list_insert(&configuration->output_configs, &output_config->link);

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_outputs(
    struct hikari_configuration *configuration, const ucl_object_t *outputs_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(outputs_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    if (!parse_output_config(configuration, cur)) {
      fprintf(stderr,
          "configuration error: failed to parse \"outputs\" configuration\n");
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

  if (configuration_obj == NULL) {
    const char *error = ucl_parser_get_error(parser);
    fprintf(stderr, "%s\n", error);
    ucl_parser_free(parser);
    return false;
  }

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
      const ucl_object_t *actions_obj =
          ucl_object_lookup(configuration_obj, "actions");
      const ucl_object_t *layouts_obj =
          ucl_object_lookup(configuration_obj, "layouts");

      if (layouts_obj != NULL && !parse_layouts(configuration, layouts_obj)) {
        fprintf(stderr, "configuration error: failed to parse \"layouts\"\n");
        goto done;
      }

      if (actions_obj != NULL && !parse_actions(configuration, actions_obj)) {
        fprintf(stderr, "configuration error: failed to parse \"actions\"\n");
        goto done;
      }

      if (!parse_bindings(configuration, actions_obj, layouts_obj, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "outputs")) {
      if (!parse_outputs(configuration, cur)) {
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

bool
hikari_configuration_reload(void)
{
  struct hikari_configuration *configuration =
      hikari_malloc(sizeof(struct hikari_configuration));

  hikari_configuration_init(configuration);

  bool success = hikari_configuration_load(configuration);

  if (success) {
    hikari_configuration_fini(hikari_configuration);
    hikari_free(hikari_configuration);
    hikari_configuration = configuration;

    struct hikari_pointer *pointer;
    wl_list_for_each (pointer, &hikari_server.pointers, server_pointers) {
      struct hikari_pointer_config *pointer_config =
          hikari_configuration_resolve_pointer_config(
              hikari_configuration, pointer->device->name);

      if (pointer_config != NULL) {
        hikari_pointer_configure(pointer, pointer_config);
      }
    }

    struct hikari_workspace *workspace;
    wl_list_for_each (workspace, &hikari_server.workspaces, server_workspaces) {
      struct hikari_sheet *sheet;
      for (int i = 0; i < HIKARI_NR_OF_SHEETS; i++) {
        sheet = workspace->sheets + i;
        struct hikari_view *view;
        wl_list_for_each (view, &sheet->views, sheet_views) {
          hikari_view_refresh_geometry(view, view->current_geometry);
        }
      }
    }

    struct hikari_output *output;
    wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
      const struct hikari_output_config *output_config =
          hikari_configuration_resolve_output(
              hikari_configuration, output->output->name);

      if (output_config != NULL) {
        hikari_output_load_background(
            output, output_config->background, output_config->background_fit);
      }
    }
  } else {
    hikari_configuration_fini(configuration);
    hikari_free(configuration);
  }

  return success;
}

void
hikari_configuration_init(struct hikari_configuration *configuration)
{
  wl_list_init(&configuration->autoconfs);
  wl_list_init(&configuration->output_configs);
  wl_list_init(&configuration->pointer_configs);
  wl_list_init(&configuration->layout_configs);
  wl_list_init(&configuration->action_configs);

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

  for (int i = 0; i < 256; i++) {
    configuration->bindings.nkeybindings[i] = 0;
    configuration->bindings.keybindings[i] = NULL;
    configuration->bindings.nmousebindings[i] = 0;
    configuration->bindings.mousebindings[i] = NULL;
  }

  configuration->border = 1;
  configuration->gap = 5;

  for (int i = 0; i < HIKARI_NR_OF_EXECS; i++) {
    hikari_exec_init(&configuration->execs[i]);
  }
}

static void
cleanup_bindings(struct hikari_keybinding **bindings, uint8_t *n)
{
  for (int i = 0; i < 256; i++) {
    struct hikari_keybinding *mod_bindings = bindings[i];

    hikari_free(mod_bindings);
  }
}

void
hikari_configuration_fini(struct hikari_configuration *configuration)
{
  cleanup_bindings(configuration->bindings.keybindings,
      configuration->bindings.nkeybindings);

  cleanup_bindings(configuration->bindings.mousebindings,
      configuration->bindings.nmousebindings);

  struct hikari_view_autoconf *autoconf, *autoconf_temp;
  wl_list_for_each_safe (
      autoconf, autoconf_temp, &configuration->autoconfs, link) {
    wl_list_remove(&autoconf->link);

    hikari_view_autoconf_fini(autoconf);
    hikari_free(autoconf);
  }

  struct hikari_output_config *output_config, *output_config_temp;
  wl_list_for_each_safe (
      output_config, output_config_temp, &configuration->output_configs, link) {
    wl_list_remove(&output_config->link);

    hikari_output_config_fini(output_config);
    hikari_free(output_config);
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

  struct hikari_layout_config *layout_config, *layout_config_temp;
  wl_list_for_each_safe (
      layout_config, layout_config_temp, &configuration->layout_configs, link) {
    wl_list_remove(&layout_config->link);

    hikari_layout_config_fini(layout_config);
    hikari_free(layout_config);
  }

  struct hikari_action_config *action_config, *action_config_temp;
  wl_list_for_each_safe (
      action_config, action_config_temp, &configuration->action_configs, link) {
    wl_list_remove(&action_config->link);

    hikari_action_config_fini(action_config);
    hikari_free(action_config);
  }

  for (int i = 0; i < HIKARI_NR_OF_EXECS; i++) {
    hikari_exec_fini(&configuration->execs[i]);
  }
}

struct hikari_output_config *
hikari_configuration_resolve_output(
    struct hikari_configuration *configuration, const char *output_name)
{
  struct hikari_output_config *output_config;
  wl_list_for_each (output_config, &configuration->output_configs, link) {
    if (!strcmp(output_config->output_name, output_name)) {
      return output_config;
    }
  }

  return NULL;
}

struct hikari_pointer_config *
hikari_configuration_resolve_pointer_config(
    struct hikari_configuration *configuration, const char *pointer_name)
{
  struct hikari_pointer_config *pointer_config;
  wl_list_for_each (pointer_config, &configuration->pointer_configs, link) {
    if (!strcmp(pointer_config->name, pointer_name)) {
      return pointer_config;
    }
  }

  return NULL;
}
