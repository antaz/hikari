#include <hikari/configuration.h>

#include <ctype.h>
#include <errno.h>

#include <ucl.h>

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_switch.h>

#include <hikari/action.h>
#include <hikari/action_config.h>
#include <hikari/binding.h>
#include <hikari/binding_config.h>
#include <hikari/color.h>
#include <hikari/command.h>
#include <hikari/exec.h>
#include <hikari/geometry.h>
#include <hikari/keyboard.h>
#include <hikari/keyboard_config.h>
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
#include <hikari/switch.h>
#include <hikari/switch_config.h>
#include <hikari/tile.h>
#include <hikari/view.h>
#include <hikari/view_config.h>
#include <hikari/workspace.h>

extern char **environ;

struct hikari_configuration *hikari_configuration = NULL;

static bool
parse_layout_func(const char *layout_func_name,
    hikari_layout_func *layout_func,
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
  struct hikari_split_container *ret = NULL;
  hikari_layout_func layout_func = NULL;
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

  ret = hikari_malloc(sizeof(struct hikari_split_container));
  hikari_split_container_init(ret, views, layout_func);

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
    hikari_layout_func layout_func;
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

    ret = hikari_malloc(sizeof(struct hikari_split_container));
    hikari_split_container_init(
        (struct hikari_split_container *)ret, views, layout_func);

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
parse_scale_value(
    const ucl_object_t *scale_value_obj, const char *name, double *scale)
{
  bool success = false;
  double ret;

  if (!ucl_object_todouble_safe(scale_value_obj, &ret)) {
    fprintf(stderr, "configuration error: expected float for \"%s\"\n", name);
    goto done;
  }

  if (ret < hikari_split_scale_min || ret > hikari_split_scale_max) {
    fprintf(stderr,
        "configuration error: \"%s\" of \"%.2f\" is not between \"0.1\" "
        "and "
        "\"0.9\"\n",
        name,
        ret);
    goto done;
  }

  success = true;

done:
  *scale = ret;

  return success;
}

static bool
parse_scale_dynamic(
    const ucl_object_t *scale_obj, struct hikari_split_scale_dynamic *scale)
{
  bool success = false;

  const ucl_object_t *min_obj = ucl_object_lookup(scale_obj, "min");
  const ucl_object_t *max_obj = ucl_object_lookup(scale_obj, "max");

  if (min_obj != NULL) {
    if (!parse_scale_value(min_obj, "min", &scale->min)) {
      goto done;
    }
  } else {
    scale->min = hikari_split_scale_min;
  }

  if (max_obj != NULL) {
    if (!parse_scale_value(max_obj, "max", &scale->max)) {
      goto done;
    }
  } else {
    scale->max = hikari_split_scale_max;
  }

  success = true;

done:

  return success;
}

static bool
parse_scale(const ucl_object_t *scale_obj, struct hikari_split_scale *scale)
{
  bool success = false;

  if (scale_obj != NULL) {
    ucl_type_t type = ucl_object_type(scale_obj);

    switch (type) {
      case UCL_FLOAT:
        scale->type = HIKARI_SPLIT_SCALE_TYPE_FIXED;
        if (!parse_scale_value(scale_obj, "scale", &scale->scale.fixed)) {
          goto done;
        }
        break;

      case UCL_OBJECT:
        scale->type = HIKARI_SPLIT_SCALE_TYPE_DYNAMIC;

        if (!parse_scale_dynamic(scale_obj, &scale->scale.dynamic)) {
          goto done;
        }
        break;

      default:
        goto done;
    }
  }

  success = true;

done:

  return success;
}

#define PARSE_SPLIT(name, NAME, first, FIRST, second, SECOND)                  \
  static bool parse_##name(                                                    \
      const ucl_object_t *name##_obj, struct hikari_split **name)              \
  {                                                                            \
    bool success = false;                                                      \
    bool found_orientation = false;                                            \
    const ucl_object_t *cur;                                                   \
    struct hikari_split_##name *ret = NULL;                                    \
    enum hikari_split_##name##_orientation orientation =                       \
        HIKARI_##NAME##_SPLIT_ORIENTATION_##FIRST;                             \
    struct hikari_split *first = NULL;                                         \
    struct hikari_split *second = NULL;                                        \
    struct hikari_split_scale scale;                                           \
    scale.type = HIKARI_SPLIT_SCALE_TYPE_FIXED;                                \
    scale.scale.fixed = hikari_split_scale_default;                            \
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
            hikari_split_free(first);                                          \
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
      hikari_split_free(first);                                                \
      fprintf(stderr,                                                          \
          "configuration error: missing \"" #second "\" for " #name            \
          " split\n");                                                         \
      goto done;                                                               \
    }                                                                          \
                                                                               \
    ret = hikari_malloc(sizeof(struct hikari_split_##name));                   \
    hikari_split_##name##_init(ret, &scale, orientation, first, second);       \
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

struct hikari_view_config *
hikari_configuration_resolve_view_config(
    struct hikari_configuration *configuration, const char *app_id)
{
  assert(app_id != NULL);

  if (app_id != NULL) {
    struct hikari_view_config *view_config;
    wl_list_for_each (view_config, &configuration->view_configs, link) {
      if (!strcmp(view_config->app_id, app_id)) {
        return view_config;
      }
    }
  }

  return NULL;
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

    if (!strcmp("selected", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_selected, color);
    } else if (!strcmp("grouped", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_grouped, color);
    } else if (!strcmp("first", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_first, color);
    } else if (!strcmp("conflict", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_conflict, color);
    } else if (!strcmp("insert", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->indicator_insert, color);
    } else if (!strcmp("active", key)) {
      if (!ucl_object_toint_safe(cur, &color)) {
        fprintf(
            stderr, "configuration error: expected integer for \"%s\"\n", key);
        goto done;
      }

      hikari_color_convert(configuration->border_active, color);
    } else if (!strcmp("inactive", key)) {
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
          "configuration error: invalid \"marks\" register \"%s\"\n",
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
          "configuration error: invalid command for \"marks\" "
          "register \"%c\"\n",
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
parse_view_configs(
    struct hikari_configuration *configuration, const ucl_object_t *obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    struct hikari_view_config *view_config =
        hikari_malloc(sizeof(struct hikari_view_config));

    hikari_view_config_init(view_config);
    wl_list_insert(&configuration->view_configs, &view_config->link);

    const char *key = ucl_object_key(cur);
    size_t keylen = strlen(key);

    view_config->app_id = hikari_malloc(keylen + 1);
    strcpy(view_config->app_id, key);

    if (!hikari_view_config_parse(view_config, cur)) {
      fprintf(stderr,
          "configuration error: failed to parse \"views\" \"%s\"\n",
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
parse_mouse_button(const char *str, uint32_t *keycode)
{
  if (!strcmp(str, "left")) {
    *keycode = BTN_LEFT;
  } else if (!strcmp(str, "right")) {
    *keycode = BTN_RIGHT;
  } else if (!strcmp(str, "middle")) {
    *keycode = BTN_MIDDLE;
  } else if (!strcmp(str, "side")) {
    *keycode = BTN_SIDE;
  } else if (!strcmp(str, "extra")) {
    *keycode = BTN_EXTRA;
  } else if (!strcmp(str, "forward")) {
    *keycode = BTN_FORWARD;
  } else if (!strcmp(str, "back")) {
    *keycode = BTN_BACK;
  } else if (!strcmp(str, "task")) {
    *keycode = BTN_TASK;
  } else {
    fprintf(stderr, "configuration error: unknown mouse button \"%s\"\n", str);
    return false;
  }

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
parse_layout(char layout_register,
    const ucl_object_t *layout_obj,
    struct hikari_split **split)
{
  struct hikari_split *ret = NULL;

  if (!parse_split(layout_obj, &ret)) {
    fprintf(stderr,
        "configuration error: failed to parse layout for register \"%c\"\n",
        layout_register);
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

    if (strlen(key) > 1 ||
        !((key[0] >= 'a' && key[0] <= 'z') || isdigit(key[0]))) {
      fprintf(stderr, "configuration error: expected layout register name\n");
      goto done;
    }

    char layout_register = key[0];

    if (!parse_layout(layout_register, cur, &split)) {
      goto done;
    }

    layout_config = hikari_malloc(sizeof(struct hikari_layout_config));
    hikari_layout_config_init(layout_config, layout_register, split);

    wl_list_insert(&configuration->layout_configs, &layout_config->link);
  }

  success = true;

done:
  return success;
}

struct hikari_split *
hikari_configuration_lookup_layout(
    struct hikari_configuration *configuration, char layout_register)
{
  struct hikari_layout_config *layout_config;
  wl_list_for_each (layout_config, &configuration->layout_configs, link) {
    if (layout_register == layout_config->layout_register) {
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
parse_keyboard_bindings(struct hikari_configuration *configuration,
    const ucl_object_t *bindings_obj)
{
  bool success = false;
  const ucl_object_t *cur;
  struct hikari_binding_config *binding_config;

  ucl_object_iter_t it = ucl_object_iterate_new(bindings_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    binding_config = hikari_malloc(sizeof(struct hikari_binding_config));
    wl_list_insert(
        &configuration->keyboard_binding_configs, &binding_config->link);

    if (!hikari_binding_config_key_parse(&binding_config->key, key)) {
      goto done;
    }

    struct hikari_action *action = &binding_config->action;
    hikari_action_init(action);

    if (!hikari_action_parse(action, &configuration->action_configs, cur)) {
      goto done;
    }
  }

  success = true;

done:

  return success;
}

static bool
finalize_keyboard_configs(struct hikari_configuration *configuration)
{
  struct hikari_keyboard_config *keyboard_config;

  if (wl_list_empty(&configuration->keyboard_configs)) {
    keyboard_config = hikari_malloc(sizeof(struct hikari_keyboard_config));
    hikari_keyboard_config_default(keyboard_config);

    wl_list_insert(&configuration->keyboard_configs, &keyboard_config->link);
  }

  wl_list_for_each (keyboard_config, &configuration->keyboard_configs, link) {
    if (!hikari_keyboard_config_compile_keymap(keyboard_config)) {
      return false;
    }
  }

  return true;
}

static bool
parse_mouse_bindings(struct hikari_configuration *configuration,
    const ucl_object_t *bindings_obj)
{
  bool success = false;
  const ucl_object_t *cur;
  struct hikari_binding_config *binding_config;

  ucl_object_iter_t it = ucl_object_iterate_new(bindings_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    binding_config = hikari_malloc(sizeof(struct hikari_binding_config));
    wl_list_insert(
        &configuration->mouse_binding_configs, &binding_config->link);

    if (!hikari_binding_config_button_parse(&binding_config->key, key)) {
      goto done;
    }

    struct hikari_action *action = &binding_config->action;
    hikari_action_init(action);

    if (!hikari_action_parse(action, &configuration->action_configs, cur)) {
      goto done;
    }
  }

  success = true;

done:

  return success;
}

static bool
parse_bindings(struct hikari_configuration *configuration,
    const ucl_object_t *bindings_obj)
{
  bool success = false;
  const ucl_object_t *cur;

  ucl_object_iter_t it = ucl_object_iterate_new(bindings_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "keyboard")) {
      if (!parse_keyboard_bindings(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "mouse")) {
      if (!parse_mouse_bindings(configuration, cur)) {
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

  return success;
}

static bool
parse_pointer_config(struct hikari_pointer_config *pointer_config,
    const ucl_object_t *pointer_config_obj)
{
  bool success = false;
  const char *pointer_name = pointer_config->name;
  const ucl_object_t *cur;

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

      if (accel < -1.0 || accel > 1.0) {
        fprintf(
            stderr, "configuration error: expected float between -1 and 1\n");
        goto done;
      }

      hikari_pointer_config_set_accel(pointer_config, accel);
    } else if (!strcmp(key, "accel-profile")) {
      const char *accel_profile;
      if (!ucl_object_tostring_safe(cur, &accel_profile)) {
        fprintf(stderr,
            "configuration error: expected string \"%s\" for "
            "\"accel-profile\"\n",
            pointer_name);
        goto done;
      }

      if (!strcmp(accel_profile, "none")) {
        hikari_pointer_config_set_accel_profile(
            pointer_config, LIBINPUT_CONFIG_ACCEL_PROFILE_NONE);
      } else if (!strcmp(accel_profile, "flat")) {
        hikari_pointer_config_set_accel_profile(
            pointer_config, LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT);
      } else if (!strcmp(accel_profile, "adaptive")) {
        hikari_pointer_config_set_accel_profile(
            pointer_config, LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE);
      } else {
        fprintf(stderr,
            "configuration error: unkown \"accel-profile\" \"%s\" for \"%s\"\n",
            accel_profile,
            pointer_name);
        goto done;
      }
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
        hikari_pointer_config_set_scroll_method(
            pointer_config, LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN);
      } else if (!strcmp(scroll_method, "no-scroll")) {
        hikari_pointer_config_set_scroll_method(
            pointer_config, LIBINPUT_CONFIG_SCROLL_NO_SCROLL);
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

      hikari_pointer_config_set_scroll_button(
          pointer_config, scroll_button_keycode);
    } else if (!strcmp(key, "disable-while-typing")) {
      bool disable_while_typing;
      if (!ucl_object_toboolean_safe(cur, &disable_while_typing)) {
        fprintf(stderr,
            "configuration error: expected boolean for "
            "\"disable-while-typing\"\n");
        goto done;
      }

      if (disable_while_typing) {
        hikari_pointer_config_set_disable_while_typing(
            pointer_config, LIBINPUT_CONFIG_DWT_ENABLED);
      } else {
        hikari_pointer_config_set_disable_while_typing(
            pointer_config, LIBINPUT_CONFIG_DWT_DISABLED);
      }
    } else if (!strcmp(key, "tap")) {
      bool tap;
      if (!ucl_object_toboolean_safe(cur, &tap)) {
        fprintf(stderr,
            "configuration error: expected boolean for "
            "\"tap\"\n");
        goto done;
      }

      if (tap) {
        hikari_pointer_config_set_tap(
            pointer_config, LIBINPUT_CONFIG_TAP_ENABLED);
      } else {
        hikari_pointer_config_set_tap(
            pointer_config, LIBINPUT_CONFIG_TAP_DISABLED);
      }
    } else if (!strcmp(key, "tap-drag")) {
      bool tap_drag;
      if (!ucl_object_toboolean_safe(cur, &tap_drag)) {
        fprintf(stderr,
            "configuration error: expected boolean for "
            "\"tap-drag\"\n");
        goto done;
      }

      if (tap_drag) {
        hikari_pointer_config_set_tap_drag(
            pointer_config, LIBINPUT_CONFIG_DRAG_ENABLED);
      } else {
        hikari_pointer_config_set_tap_drag(
            pointer_config, LIBINPUT_CONFIG_DRAG_DISABLED);
      }
    } else if (!strcmp(key, "tap-drag-lock")) {
      bool tap_drag_lock;
      if (!ucl_object_toboolean_safe(cur, &tap_drag_lock)) {
        fprintf(stderr,
            "configuration error: expected boolean for "
            "\"tap-drag-lock\"\n");
        goto done;
      }

      if (tap_drag_lock) {
        hikari_pointer_config_set_tap_drag_lock(
            pointer_config, LIBINPUT_CONFIG_DRAG_LOCK_ENABLED);
      } else {
        hikari_pointer_config_set_tap_drag_lock(
            pointer_config, LIBINPUT_CONFIG_DRAG_LOCK_DISABLED);
      }
    } else if (!strcmp(key, "middle-emulation")) {
      bool middle_emulation;
      if (!ucl_object_toboolean_safe(cur, &middle_emulation)) {
        fprintf(stderr,
            "configuration error: expected boolean for "
            "\"middle-emulation\"\n");
        goto done;
      }

      if (middle_emulation) {
        hikari_pointer_config_set_middle_emulation(
            pointer_config, LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED);
      } else {
        hikari_pointer_config_set_middle_emulation(
            pointer_config, LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED);
      }
    } else if (!strcmp(key, "natural-scrolling")) {
      bool natural_scrolling;
      if (!ucl_object_toboolean_safe(cur, &natural_scrolling)) {
        fprintf(stderr,
            "configuration error: expected boolean for "
            "\"natural-scrolling\"\n");
        goto done;
      }

      hikari_pointer_config_set_natural_scrolling(
          pointer_config, natural_scrolling);
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
  struct hikari_pointer_config *pointer_config;

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *pointer_name = ucl_object_key(cur);

    pointer_config = hikari_malloc(sizeof(struct hikari_pointer_config));
    hikari_pointer_config_init(pointer_config, pointer_name);

    wl_list_insert(&configuration->pointer_configs, &pointer_config->link);

    if (!parse_pointer_config(pointer_config, cur)) {
      goto done;
    }
  }

  struct hikari_pointer_config *default_config =
      hikari_configuration_resolve_pointer_config(configuration, "*");

  if (default_config != NULL) {
    wl_list_for_each (pointer_config, &configuration->pointer_configs, link) {
      if (!!strcmp(pointer_config->name, "*")) {
        hikari_pointer_config_merge(pointer_config, default_config);
      }
    }
  }

  success = true;

done:

  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_keyboards(struct hikari_configuration *configuration,
    const ucl_object_t *keyboards_obj)
{
  bool success = false;
  const ucl_object_t *cur;
  struct hikari_keyboard_config *keyboard_config;

  ucl_object_iter_t it = ucl_object_iterate_new(keyboards_obj);
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *keyboard_name = ucl_object_key(cur);

    keyboard_config = hikari_malloc(sizeof(struct hikari_keyboard_config));
    hikari_keyboard_config_init(keyboard_config, keyboard_name);

    wl_list_insert(&configuration->keyboard_configs, &keyboard_config->link);

    if (!hikari_keyboard_config_parse(keyboard_config, cur)) {
      goto done;
    }
  }

  struct hikari_keyboard_config *default_config =
      hikari_configuration_resolve_keyboard_config(configuration, "*");
  if (default_config == NULL) {
    default_config = hikari_malloc(sizeof(struct hikari_keyboard_config));
    hikari_keyboard_config_default(default_config);

    wl_list_insert(&configuration->keyboard_configs, &default_config->link);
  }

  wl_list_for_each (keyboard_config, &configuration->keyboard_configs, link) {
    if (!!strcmp(keyboard_config->keyboard_name, "*")) {
      hikari_keyboard_config_merge(keyboard_config, default_config);
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_switches(struct hikari_configuration *configuration,
    const ucl_object_t *switches_obj)
{
  bool success = false;
  const ucl_object_t *cur;
  struct hikari_switch_config *switch_config;

  ucl_object_iter_t it = ucl_object_iterate_new(switches_obj);
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);

    switch_config = hikari_malloc(sizeof(struct hikari_switch_config));
    wl_list_insert(&configuration->switch_configs, &switch_config->link);

    switch_config->switch_name = strdup(key);

    if (!hikari_action_parse(
            &switch_config->action, &configuration->action_configs, cur)) {
      goto done;
    }
  }

  success = true;

done:

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
    } else if (!strcmp(key, "keyboards")) {
      if (!parse_keyboards(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "switches")) {
      if (!parse_switches(configuration, cur)) {
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
parse_background(const ucl_object_t *background_obj,
    char **background,
    enum hikari_background_fit *fit)
{
  bool success = false;

  ucl_object_iter_t it = ucl_object_iterate_new(background_obj);
  bool has_background = false;

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);
    if (!strcmp(key, "path")) {
      has_background = true;
      *background = copy_in_config_string(cur);
    } else if (!strcmp(key, "fit")) {
      const char *fit_value;
      if (!ucl_object_tostring_safe(cur, &fit_value)) {
        fprintf(stderr,
            "configuration error: expected string for \"background\" "
            "\"fit\"\n");
        goto done;
      }
      if (!strcmp(fit_value, "center")) {
        *fit = HIKARI_BACKGROUND_CENTER;
      } else if (!strcmp(fit_value, "stretch")) {
        *fit = HIKARI_BACKGROUND_STRETCH;
      } else if (!strcmp(fit_value, "tile")) {
        *fit = HIKARI_BACKGROUND_TILE;
      } else {
        fprintf(stderr,
            "configuration error: unexpected \"background\" \"fit\" \"%s\"\n",
            fit_value);
        goto done;
      }
    } else {
      fprintf(stderr,
          "configuration error: unknown \"background\" configuration key "
          "\"%s\"\n",
          key);
      goto done;
    }
  }

  if (!has_background) {
    fprintf(
        stderr, "configuration error: missing \"path\" for \"background\"\n");
    goto done;
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
parse_output_config(struct hikari_output_config *output_config,
    const ucl_object_t *output_config_obj)

{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(output_config_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "background")) {
      ucl_type_t type = ucl_object_type(cur);

      if (type == UCL_STRING) {
        char *background = copy_in_config_string(cur);

        if (background == NULL) {
          fprintf(
              stderr, "configuration error: invalid \"background\" value\n");
          goto done;
        }

        hikari_output_config_set_background(output_config, background);
      } else if (type == UCL_OBJECT) {
        char *background;
        enum hikari_background_fit background_fit;

        if (!parse_background(cur, &background, &background_fit)) {
          goto done;
        }

        hikari_output_config_set_background(output_config, background);
        hikari_output_config_set_background_fit(output_config, background_fit);
      } else {
        fprintf(stderr,
            "configuration error: expected string or object for "
            "\"background\"\n");
        goto done;
      }
    } else if (!strcmp(key, "position")) {
      struct hikari_position_config position;
      if (!hikari_position_config_absolute_parse(&position, cur)) {
        fprintf(stderr,
            "configuration error: could not parse \"output\" \"position\"");
        goto done;
      }

      hikari_output_config_set_position(output_config, position);
    } else {
      fprintf(stderr,
          "configuration error: unknown \"outputs\" configuration key \"%s\"\n",
          key);
    }
  }

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
  struct hikari_output_config *output_config;

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *output_name = ucl_object_key(cur);

    output_config = hikari_malloc(sizeof(struct hikari_output_config));
    hikari_output_config_init(output_config, output_name);

    wl_list_insert(&configuration->output_configs, &output_config->link);

    if (!parse_output_config(output_config, cur)) {
      fprintf(stderr,
          "configuration error: failed to parse \"outputs\" configuration\n");
      goto done;
    }
  }

  success = true;

  struct hikari_output_config *default_config =
      hikari_configuration_resolve_output_config(configuration, "*");

  if (default_config != NULL) {
    wl_list_for_each (output_config, &configuration->output_configs, link) {
      if (!!strcmp(output_config->output_name, "*")) {
        hikari_output_config_merge(output_config, default_config);
      }
    }
  }

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
parse_step(
    struct hikari_configuration *configuration, const ucl_object_t *step_obj)
{
  int64_t step;

  if (!ucl_object_toint_safe(step_obj, &step)) {
    fprintf(stderr, "configuration error: expected integer for \"step\"\n");
    return false;
  }

  configuration->step = step;

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

static bool
parse_ui(struct hikari_configuration *configuration, const ucl_object_t *ui_obj)
{
  bool success = false;
  ucl_object_iter_t it = ucl_object_iterate_new(ui_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "colorscheme")) {
      if (!parse_colorscheme(configuration, cur)) {
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
    } else if (!strcmp(key, "step")) {
      if (!parse_step(configuration, cur)) {
        goto done;
      }
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
set_env_vars(struct ucl_parser *parser)
{
  for (char **current_var = environ; *current_var != NULL; ++current_var) {
    const char *separator = strchr(*current_var, '=');
    if (separator == NULL) {
      continue;
    }

    const size_t name_length = separator - *current_var;
    char *name = hikari_malloc(name_length + 1);
    if (name == NULL) {
      fprintf(stderr, "Could not allocate enough memory :(\n");
      return false;
    }
    strncpy(name, *current_var, name_length);
    name[name_length] = '\0';
    ucl_parser_register_variable(parser, name, separator + 1);
    hikari_free(name);
  }

  return true;
}

bool
hikari_configuration_load(
    struct hikari_configuration *configuration, char *config_path)
{
  struct ucl_parser *parser = ucl_parser_new(0);
  if (!set_env_vars(parser)) {
    ucl_parser_free(parser);
    return false;
  }
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

  const ucl_object_t *actions_obj =
      ucl_object_lookup(configuration_obj, "actions");
  if (actions_obj != NULL && !parse_actions(configuration, actions_obj)) {
    fprintf(stderr, "configuration error: failed to parse \"actions\"\n");
    goto done;
  }

  const ucl_object_t *layouts_obj =
      ucl_object_lookup(configuration_obj, "layouts");
  if (layouts_obj != NULL && !parse_layouts(configuration, layouts_obj)) {
    fprintf(stderr, "configuration error: failed to parse \"layouts\"\n");
    goto done;
  }

  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp(key, "ui")) {
      if (!parse_ui(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "views")) {
      if (!parse_view_configs(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "marks")) {
      if (!parse_execute(configuration, cur)) {
        goto done;
      }
    } else if (!strcmp(key, "bindings")) {
      if (!parse_bindings(configuration, cur)) {
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
    } else if (!!strcmp(key, "actions") && !!strcmp(key, "layouts")) {
      fprintf(stderr,
          "configuration error: unkown configuration section \"%s\"\n",
          key);
      goto done;
    }
  }

  if (!finalize_keyboard_configs(configuration)) {
    goto done;
  }

  success = true;

done:
  ucl_object_iterate_free(it);
  ucl_object_unref(configuration_obj);
  ucl_parser_free(parser);

  return success;
}

bool
hikari_configuration_reload(char *config_path)
{
  struct hikari_configuration *configuration =
      hikari_malloc(sizeof(struct hikari_configuration));

  hikari_configuration_init(configuration);

  bool success = hikari_configuration_load(configuration, config_path);

  if (success) {
    if (hikari_server.workspace->focus_view != NULL) {
      hikari_indicator_damage(
          &hikari_server.indicator, hikari_server.workspace->focus_view);
    }

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

    hikari_cursor_configure_bindings(
        &hikari_server.cursor, &configuration->mouse_binding_configs);

    struct hikari_keyboard *keyboard;
    wl_list_for_each (keyboard, &hikari_server.keyboards, server_keyboards) {
      struct hikari_keyboard_config *keyboard_config =
          hikari_configuration_resolve_keyboard_config(
              hikari_configuration, keyboard->device->name);

      assert(keyboard_config != NULL);
      hikari_keyboard_configure(keyboard, keyboard_config);

      hikari_keyboard_configure_bindings(
          keyboard, &configuration->keyboard_binding_configs);
    }

    struct hikari_output *output;
    wl_list_for_each (output, &hikari_server.outputs, server_outputs) {
      struct hikari_view *view;
      wl_list_for_each (view, &output->views, output_views) {
        hikari_view_refresh_geometry(view, view->current_geometry);
      }

      struct hikari_output_config *output_config =
          hikari_configuration_resolve_output_config(
              hikari_configuration, output->wlr_output->name);

      if (output_config != NULL) {
        if (output_config->position.value.type ==
            HIKARI_POSITION_CONFIG_TYPE_ABSOLUTE) {
          int x = output_config->position.value.config.absolute.x;
          int y = output_config->position.value.config.absolute.y;

          if (output->geometry.x != x || output->geometry.y != y) {
            hikari_output_move(output, x, y);
          }
        }

        if (output_config->background.value != NULL) {
          hikari_output_load_background(output,
              output_config->background.value,
              output_config->background_fit.value);
        }
      }
    }

    struct hikari_switch *swtch;
    wl_list_for_each (swtch, &hikari_server.switches, server_switches) {
      struct hikari_switch_config *switch_config =
          hikari_configuration_resolve_switch_config(
              hikari_configuration, swtch->device->name);

      if (switch_config != NULL) {
        hikari_switch_configure(swtch, switch_config);
      } else {
        hikari_switch_reset(swtch);
      }
    }

    if (hikari_server.workspace->focus_view != NULL) {
      hikari_indicator_update(
          &hikari_server.indicator, hikari_server.workspace->focus_view);
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
  wl_list_init(&configuration->view_configs);
  wl_list_init(&configuration->output_configs);
  wl_list_init(&configuration->pointer_configs);
  wl_list_init(&configuration->keyboard_configs);
  wl_list_init(&configuration->layout_configs);
  wl_list_init(&configuration->action_configs);
  wl_list_init(&configuration->keyboard_binding_configs);
  wl_list_init(&configuration->mouse_binding_configs);
  wl_list_init(&configuration->switch_configs);

  hikari_color_convert(configuration->clear, 0x282C34);
  hikari_color_convert(configuration->foreground, 0x000000);
  hikari_color_convert(configuration->indicator_selected, 0xF5E094);
  hikari_color_convert(configuration->indicator_grouped, 0xFDAF53);
  hikari_color_convert(configuration->indicator_first, 0xB8E673);
  hikari_color_convert(configuration->indicator_conflict, 0xED6B32);
  hikari_color_convert(configuration->indicator_insert, 0xE3C3FA);
  hikari_color_convert(configuration->border_active, 0xFFFFFF);
  hikari_color_convert(configuration->border_inactive, 0x465457);

  hikari_font_init(&configuration->font, "monospace 10");

  configuration->border = 1;
  configuration->gap = 5;
  configuration->step = 100;

  for (int i = 0; i < HIKARI_NR_OF_EXECS; i++) {
    hikari_exec_init(&configuration->execs[i]);
  }
}

void
hikari_configuration_fini(struct hikari_configuration *configuration)
{
  struct hikari_view_config *view_config, *view_config_temp;
  wl_list_for_each_safe (
      view_config, view_config_temp, &configuration->view_configs, link) {
    wl_list_remove(&view_config->link);

    hikari_view_config_fini(view_config);
    hikari_free(view_config);
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

  struct hikari_keyboard_config *keyboard_config, *keyboard_config_temp;
  wl_list_for_each_safe (keyboard_config,
      keyboard_config_temp,
      &configuration->keyboard_configs,
      link) {
    wl_list_remove(&keyboard_config->link);

    hikari_keyboard_config_fini(keyboard_config);
    hikari_free(keyboard_config);
  }

  struct hikari_switch_config *switch_config, *switch_config_temp;
  wl_list_for_each_safe (
      switch_config, switch_config_temp, &configuration->switch_configs, link) {
    wl_list_remove(&switch_config->link);

    hikari_switch_config_fini(switch_config);
    hikari_free(switch_config);
  }

  struct hikari_binding_config *binding_config, *binding_config_temp;
  wl_list_for_each_safe (binding_config,
      binding_config_temp,
      &configuration->keyboard_binding_configs,
      link) {
    wl_list_remove(&binding_config->link);
    hikari_free(binding_config);
  }
  wl_list_for_each_safe (binding_config,
      binding_config_temp,
      &configuration->mouse_binding_configs,
      link) {
    wl_list_remove(&binding_config->link);
    hikari_free(binding_config);
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
hikari_configuration_resolve_output_config(
    struct hikari_configuration *configuration, const char *output_name)
{
  struct hikari_output_config *output_config;
  wl_list_for_each (output_config, &configuration->output_configs, link) {
    if (!strcmp(output_config->output_name, output_name)) {
      return output_config;
    }
  }

  wl_list_for_each (output_config, &configuration->output_configs, link) {
    if (!strcmp(output_config->output_name, "*")) {
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

  wl_list_for_each (pointer_config, &configuration->pointer_configs, link) {
    if (!strcmp(pointer_config->name, "*")) {
      return pointer_config;
    }
  }

  return NULL;
}

struct hikari_switch_config *
hikari_configuration_resolve_switch_config(
    struct hikari_configuration *configuration, const char *switch_name)
{
  struct hikari_switch_config *switch_config;
  wl_list_for_each (switch_config, &configuration->switch_configs, link) {
    if (!strcmp(switch_config->switch_name, switch_name)) {
      return switch_config;
    }
  }

  return NULL;
}

struct hikari_keyboard_config *
hikari_configuration_resolve_keyboard_config(
    struct hikari_configuration *configuration, const char *keyboard_name)
{
  struct hikari_keyboard_config *keyboard_config;
  wl_list_for_each (keyboard_config, &configuration->keyboard_configs, link) {
    if (!strcmp(keyboard_config->keyboard_name, keyboard_name)) {
      return keyboard_config;
    }
  }

  wl_list_for_each (keyboard_config, &configuration->keyboard_configs, link) {
    if (!strcmp(keyboard_config->keyboard_name, "*")) {
      return keyboard_config;
    }
  }

  return NULL;
}
