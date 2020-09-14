#include <hikari/keyboard_config.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define HIKARI_KEYBOARD_CONFIG_DEFAULT_REPEAT_RATE 25
#define HIKARI_KEYBOARD_CONFIG_DEFAULT_REPEAT_DELAY 600

static void
init_default_repeat(struct hikari_keyboard_config *keyboard_config)
{
  hikari_keyboard_config_set_repeat_rate(
      keyboard_config, HIKARI_KEYBOARD_CONFIG_DEFAULT_REPEAT_RATE);
  hikari_keyboard_config_set_repeat_delay(
      keyboard_config, HIKARI_KEYBOARD_CONFIG_DEFAULT_REPEAT_DELAY);
}

static bool
parse_xkb_rules(
    struct hikari_xkb_config *xkb_config, const ucl_object_t *xkb_obj)
{
  bool success = false;
  const ucl_object_t *cur;

  ucl_object_iter_t it = ucl_object_iterate_new(xkb_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp("rules", key)) {
      const char *rules;
      if (!ucl_object_tostring_safe(cur, &rules)) {
        fprintf(stderr, "configuration error: expected string for \"rules\"\n");
        goto done;
      }

      hikari_xkb_config_set_rules(xkb_config, strdup(rules));
    } else if (!strcmp("model", key)) {
      const char *model;
      if (!ucl_object_tostring_safe(cur, &model)) {
        fprintf(stderr, "configuration error: expected string for \"model\"\n");
        goto done;
      }

      hikari_xkb_config_set_model(xkb_config, strdup(model));
    } else if (!strcmp("layout", key)) {
      const char *layout;
      if (!ucl_object_tostring_safe(cur, &layout)) {
        fprintf(
            stderr, "configuration error: expected string for \"layout\"\n");
        goto done;
      }

      hikari_xkb_config_set_layout(xkb_config, strdup(layout));
    } else if (!strcmp("variant", key)) {
      const char *variant;
      if (!ucl_object_tostring_safe(cur, &variant)) {
        fprintf(
            stderr, "configuration error: expected string for \"variant\"\n");
        goto done;
      }

      hikari_xkb_config_set_variant(xkb_config, strdup(variant));
    } else if (!strcmp("options", key)) {
      const char *options;
      if (!ucl_object_tostring_safe(cur, &options)) {
        fprintf(
            stderr, "configuration error: expected string for \"options\"\n");
        goto done;
      }

      hikari_xkb_config_set_options(xkb_config, strdup(options));
    } else {
      fprintf(stderr, "configuration error: invalid \"xkb\" key \"%s\"\n", key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static bool
load_xkb_file(struct hikari_xkb *xkb, const char *xkb_file)
{
  bool success = false;

  FILE *keymap_file = fopen(xkb_file, "r");

  if (!keymap_file) {
    fprintf(stderr,
        "configuration error: failed to open \"xkb\" file \"%s\"\n",
        xkb_file);
    goto done;
  }

  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap = xkb_keymap_new_from_file(context,
      keymap_file,
      XKB_KEYMAP_FORMAT_TEXT_V1,
      XKB_KEYMAP_COMPILE_NO_FLAGS);
  xkb_context_unref(context);

  if (keymap == NULL) {
    goto done;
  }

  xkb->type = HIKARI_XKB_TYPE_RULES;
  xkb->value.keymap = keymap;

  success = true;

done:

  return success;
}

bool
hikari_keyboard_config_parse(struct hikari_keyboard_config *keyboard_config,
    const ucl_object_t *keyboard_config_obj)
{
  bool success = false;
  const ucl_object_t *cur;

  ucl_object_iter_t it = ucl_object_iterate_new(keyboard_config_obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp("xkb", key)) {
      ucl_type_t type = ucl_object_type(cur);

      if (type == UCL_OBJECT) {
        keyboard_config->xkb.type = HIKARI_XKB_TYPE_RULES;

        if (!parse_xkb_rules(&keyboard_config->xkb.value.rules, cur)) {
          fprintf(stderr, "configuration error: failed to parse xkb rules\n");
          goto done;
        }
      } else if (type == UCL_STRING) {
        const char *xkb_file;
        if (!ucl_object_tostring_safe(cur, &xkb_file) ||
            !load_xkb_file(&keyboard_config->xkb, xkb_file)) {
          goto done;
        }
      } else {
        fprintf(stderr,
            "configuration error: expected string or object for \"xkb\"\n");
        goto done;
      }
    } else if (!strcmp("repeat-rate", key)) {
      int64_t repeat_rate;

      if (!ucl_object_toint_safe(cur, &repeat_rate) || repeat_rate < 0) {
        fprintf(stderr,
            "configuration error: expected positive integer \"repeat-rate\"\n");
        goto done;
      }

      hikari_keyboard_config_set_repeat_rate(keyboard_config, repeat_rate);
    } else if (!strcmp("repeat-delay", key)) {
      int64_t repeat_delay;

      if (!ucl_object_toint_safe(cur, &repeat_delay) || repeat_delay < 0) {
        fprintf(stderr,
            "configuration error: expected positive integer "
            "\"repeat-delay\"\n");
        goto done;
      }

      hikari_keyboard_config_set_repeat_delay(keyboard_config, repeat_delay);
    } else {
      fprintf(stderr,
          "configuration error: invalid \"keyboard\" key \"%s\"\n",
          key);
      goto done;
    }
  }

  success = true;

done:
  ucl_object_iterate_free(it);

  return success;
}

static void
xkb_init(struct hikari_xkb_config *xkb_config)
{
  hikari_xkb_config_init_rules(xkb_config, NULL);
  hikari_xkb_config_init_model(xkb_config, NULL);
  hikari_xkb_config_init_layout(xkb_config, NULL);
  hikari_xkb_config_init_variant(xkb_config, NULL);
  hikari_xkb_config_init_options(xkb_config, NULL);
}

void
hikari_keyboard_config_init(
    struct hikari_keyboard_config *keyboard_config, const char *keyboard_name)
{
  keyboard_config->xkb.type = HIKARI_XKB_TYPE_RULES;

  xkb_init(&keyboard_config->xkb.value.rules);

  keyboard_config->keyboard_name = strdup(keyboard_name);

  if (!strcmp(keyboard_name, "*")) {
    init_default_repeat(keyboard_config);
  } else {
    hikari_keyboard_config_init_repeat_rate(
        keyboard_config, HIKARI_KEYBOARD_CONFIG_DEFAULT_REPEAT_RATE);
    hikari_keyboard_config_init_repeat_delay(
        keyboard_config, HIKARI_KEYBOARD_CONFIG_DEFAULT_REPEAT_DELAY);
  }
}

static void
xkb_fini(struct hikari_xkb_config *xkb_config)
{
  free(xkb_config->rules.value);
  free(xkb_config->model.value);
  free(xkb_config->layout.value);
  free(xkb_config->variant.value);
  free(xkb_config->options.value);
}

void
hikari_keyboard_config_fini(struct hikari_keyboard_config *keyboard_config)
{
  switch (keyboard_config->xkb.type) {
    case HIKARI_XKB_TYPE_KEYMAP:
      xkb_keymap_unref(keyboard_config->xkb.value.keymap);
      break;

    case HIKARI_XKB_TYPE_RULES:
      xkb_fini(&keyboard_config->xkb.value.rules);
      break;
  }

  free(keyboard_config->keyboard_name);
}

static void
default_init(struct hikari_xkb_config *xkb_config)
{
  char *rules = getenv("XKB_DEFAULT_RULES");
  char *model = getenv("XKB_DEFAULT_MODEL");
  char *layout = getenv("XKB_DEFAULT_LAYOUT");
  char *variant = getenv("XKB_DEFAULT_VARIANT");
  char *options = getenv("XKB_DEFAULT_OPTIONS");

  if (rules == NULL) {
    hikari_xkb_config_set_rules(xkb_config, strdup("evdev"));
  } else {
    hikari_xkb_config_set_rules(xkb_config, strdup(rules));
  }

  if (model != NULL) {
    hikari_xkb_config_set_model(xkb_config, strdup(model));
  }

  if (layout != NULL) {
    hikari_xkb_config_set_layout(xkb_config, strdup(layout));
  }

  if (variant != NULL) {
    hikari_xkb_config_set_variant(xkb_config, strdup(variant));
  }

  if (options != NULL) {
    hikari_xkb_config_set_options(xkb_config, strdup(options));
  }
}

void
hikari_keyboard_config_default(struct hikari_keyboard_config *keyboard_config)
{
  keyboard_config->xkb.type = HIKARI_XKB_TYPE_RULES;

  xkb_init(&keyboard_config->xkb.value.rules);
  default_init(&keyboard_config->xkb.value.rules);

  keyboard_config->keyboard_name = malloc(2 * sizeof(char));
  strcpy(keyboard_config->keyboard_name, "*");

  init_default_repeat(keyboard_config);
}

static bool
has_set_options(struct hikari_xkb_config *xkb_config)
{
  return hikari_xkb_config_has_rules(xkb_config) ||
         hikari_xkb_config_has_model(xkb_config) ||
         hikari_xkb_config_has_layout(xkb_config) ||
         hikari_xkb_config_has_variant(xkb_config) ||
         hikari_xkb_config_has_options(xkb_config);
}

#define MERGE(option)                                                          \
  if (hikari_xkb_config_merge_##option(config_xkb, default_xkb) &&             \
      default_xkb->option.value != NULL) {                                     \
    config_xkb->option.value = strdup(default_xkb->option.value);              \
  }

void
hikari_keyboard_config_merge(struct hikari_keyboard_config *keyboard_config,
    struct hikari_keyboard_config *default_config)
{
  hikari_keyboard_config_merge_repeat_rate(keyboard_config, default_config);
  hikari_keyboard_config_merge_repeat_delay(keyboard_config, default_config);

  if (default_config->xkb.type == HIKARI_XKB_TYPE_KEYMAP) {
    if (keyboard_config->xkb.type == HIKARI_XKB_TYPE_RULES) {
      if (!has_set_options(&keyboard_config->xkb.value.rules)) {
        keyboard_config->xkb.type = HIKARI_XKB_TYPE_KEYMAP;
        keyboard_config->xkb.value.keymap =
            xkb_keymap_ref(default_config->xkb.value.keymap);
      }
    }
  } else {
    assert(default_config->xkb.type == HIKARI_XKB_TYPE_RULES);

    if (keyboard_config->xkb.type == HIKARI_XKB_TYPE_RULES) {
      struct hikari_xkb_config *config_xkb = &keyboard_config->xkb.value.rules;
      struct hikari_xkb_config *default_xkb = &default_config->xkb.value.rules;

      MERGE(rules);
      MERGE(model);
      MERGE(layout);
      MERGE(variant);
      MERGE(options);
    }
  }
}
#undef MERGE

static struct xkb_keymap *
compile_keymap(struct hikari_xkb_config *xkb_config)
{
  struct xkb_rule_names rules = { 0 };

  rules.rules = xkb_config->rules.value;
  rules.model = xkb_config->model.value;
  rules.layout = xkb_config->layout.value;
  rules.variant = xkb_config->variant.value;
  rules.options = xkb_config->options.value;

  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap =
      xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
  xkb_context_unref(context);

  return keymap;
}

bool
hikari_keyboard_config_compile_keymap(
    struct hikari_keyboard_config *keyboard_config)
{
  if (keyboard_config->xkb.type == HIKARI_XKB_TYPE_KEYMAP) {
    return true;
  }

  assert(keyboard_config->xkb.type == HIKARI_XKB_TYPE_RULES);

  struct xkb_keymap *keymap = compile_keymap(&keyboard_config->xkb.value.rules);

  if (keymap == NULL) {
    return false;
  }

  xkb_fini(&keyboard_config->xkb.value.rules);

  keyboard_config->xkb.type = HIKARI_XKB_TYPE_KEYMAP;
  keyboard_config->xkb.value.keymap = keymap;

  return true;
}
