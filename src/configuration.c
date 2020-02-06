#include <hikari/configuration.h>

#include <ucl.h>

#include <wlr/types/wlr_cursor.h>

#include <hikari/background.h>
#include <hikari/color.h>
#include <hikari/command.h>
#include <hikari/exec.h>
#include <hikari/keybinding.h>
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
parse_modifier_mask(const char *str, uint8_t *result)
{
  size_t len = strlen(str);
  uint8_t mask = 0;
  int pos;

  for (pos = 0; pos < len; pos++) {
    char c = str[pos];
    if (c == '-') {
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
      fprintf(stderr, "config error: unkown modifier %c in %s\n", c, str);
      return false;
    }
  }

  *result = mask;
  return true;
}

static struct hikari_split *
parse_container(const ucl_object_t *container_obj)
{
  const ucl_object_t *layout_type = ucl_object_lookup(container_obj, "type");

  if (layout_type != NULL) {
    layout_func_t layout_func = NULL;
    const char *str = ucl_object_tostring(layout_type);

    if (!strcmp(str, "vertical")) {
      layout_func = hikari_sheet_vertical_layout;
    } else if (!strcmp(str, "horizontal")) {
      layout_func = hikari_sheet_horizontal_layout;
    } else if (!strcmp(str, "full")) {
      layout_func = hikari_sheet_full_layout;
    } else if (!strcmp(str, "grid")) {
      layout_func = hikari_sheet_grid_layout;
    } else {
      assert(false);
    }

    struct hikari_container *container =
        hikari_malloc(sizeof(struct hikari_container));

    // TODO fix upper bound
    int views;
    const ucl_object_t *views_obj = ucl_object_lookup(container_obj, "views");
    if (views_obj != NULL) {
      views = ucl_object_toint(views_obj);
    } else {
      views = 1000;
    }

    hikari_container_init(container, views, layout_func);

    return (struct hikari_split *)container;
  } else {
    return NULL;
  }
}

static struct hikari_split *
parse_vertical(const ucl_object_t *vertical_obj);

static struct hikari_split *
parse_horizontal(const ucl_object_t *vertical_obj);

static struct hikari_split *
parse_split(const ucl_object_t *split)
{
  struct hikari_split *ret = NULL;
  ucl_object_iter_t it = ucl_object_iterate_new(split);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);
    if (!strcmp(key, "container")) {
      ret = parse_container(cur);
    } else if (!strcmp(key, "vertical")) {
      ret = parse_vertical(cur);
    } else if (!strcmp(key, "horizontal")) {
      ret = parse_horizontal(cur);
    } else {
      assert(false);
    }
  }

  ucl_object_iterate_free(it);

  return ret;
}

static struct hikari_split *
parse_vertical(const ucl_object_t *vertical_obj)
{
  const ucl_object_t *ratio_obj = ucl_object_lookup(vertical_obj, "ratio");
  const ucl_object_t *right_obj = ucl_object_lookup(vertical_obj, "right");
  const ucl_object_t *left_obj = ucl_object_lookup(vertical_obj, "left");

  double ratio;

  if (ratio_obj != NULL) {
    ratio = ucl_object_todouble(ratio_obj);
  } else {
    ratio = 0.5;
  }

  struct hikari_split *right;
  if (right_obj != NULL) {
    right = parse_split(right_obj);
  } else {
    right = NULL;
    assert(false);
  }

  struct hikari_split *left;
  if (left_obj != NULL) {
    left = parse_split(left_obj);
  } else {
    left = NULL;
    assert(false);
  }

  struct hikari_vertical_split *vertical =
      hikari_malloc(sizeof(struct hikari_vertical_split));

  hikari_vertical_split_init(vertical, ratio, left, right);

  return (struct hikari_split *)vertical;
}

static struct hikari_split *
parse_horizontal(const ucl_object_t *horizontal_obj)
{
  const ucl_object_t *ratio_obj = ucl_object_lookup(horizontal_obj, "ratio");
  const ucl_object_t *top_obj = ucl_object_lookup(horizontal_obj, "top");
  const ucl_object_t *bottom_obj = ucl_object_lookup(horizontal_obj, "bottom");

  double ratio;

  if (ratio_obj != NULL) {
    ratio = ucl_object_todouble(ratio_obj);
  } else {
    ratio = 0.5;
  }

  struct hikari_split *top;
  if (top_obj != NULL) {
    top = parse_split(top_obj);
  } else {
    top = NULL;
    assert(false);
  }

  struct hikari_split *bottom;
  if (bottom_obj != NULL) {
    bottom = parse_split(bottom_obj);
  } else {
    bottom = NULL;
    assert(false);
  }

  struct hikari_horizontal_split *horizontal =
      hikari_malloc(sizeof(struct hikari_horizontal_split));

  hikari_horizontal_split_init(horizontal, ratio, top, bottom);

  return (struct hikari_split *)horizontal;
}

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

    if (strlen(view_autoconf->group_name) > 0) {
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
    return NULL;
  }
}

static void
parse_colorscheme(
    struct hikari_configuration *configuration, const ucl_object_t *obj)
{
  ucl_object_iter_t it = ucl_object_iterate_new(obj);

  const ucl_object_t *cur;
  int64_t color;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!strcmp("indicator_selected", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->indicator_selected, color);
    } else if (!strcmp("indicator_grouped", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->indicator_grouped, color);
    } else if (!strcmp("indicator_first", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->indicator_first, color);
    } else if (!strcmp("indicator_conflict", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->indicator_conflict, color);
    } else if (!strcmp("indicator_insert", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->indicator_insert, color);
    } else if (!strcmp("border_active", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->border_active, color);
    } else if (!strcmp("border_inactive", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->border_inactive, color);
    } else if (!strcmp("foreground", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->foreground, color);
    } else if (!strcmp("background", key)) {
      ucl_object_toint_safe(cur, &color);
      hikari_color_convert(configuration->clear, color);
    }
  }

  ucl_object_iterate_free(it);
}

static bool
parse_execute(
    struct hikari_configuration *configuration, const ucl_object_t *obj)
{
  bool success = true;
  const ucl_object_t *cur;
  const char *key;

  ucl_object_iter_t it = ucl_object_iterate_new(obj);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    key = ucl_object_key(cur);

    struct hikari_exec *execute = NULL;

    if (!strcmp("a", key)) {
      execute = HIKARI_EXEC_a;
    } else if (!strcmp("b", key)) {
      execute = HIKARI_EXEC_b;
    } else if (!strcmp("c", key)) {
      execute = HIKARI_EXEC_c;
    } else if (!strcmp("d", key)) {
      execute = HIKARI_EXEC_d;
    } else if (!strcmp("e", key)) {
      execute = HIKARI_EXEC_e;
    } else if (!strcmp("f", key)) {
      execute = HIKARI_EXEC_f;
    } else if (!strcmp("g", key)) {
      execute = HIKARI_EXEC_g;
    } else if (!strcmp("h", key)) {
      execute = HIKARI_EXEC_h;
    } else if (!strcmp("i", key)) {
      execute = HIKARI_EXEC_i;
    } else if (!strcmp("j", key)) {
      execute = HIKARI_EXEC_j;
    } else if (!strcmp("k", key)) {
      execute = HIKARI_EXEC_k;
    } else if (!strcmp("l", key)) {
      execute = HIKARI_EXEC_l;
    } else if (!strcmp("m", key)) {
      execute = HIKARI_EXEC_m;
    } else if (!strcmp("n", key)) {
      execute = HIKARI_EXEC_n;
    } else if (!strcmp("o", key)) {
      execute = HIKARI_EXEC_o;
    } else if (!strcmp("p", key)) {
      execute = HIKARI_EXEC_p;
    } else if (!strcmp("q", key)) {
      execute = HIKARI_EXEC_q;
    } else if (!strcmp("r", key)) {
      execute = HIKARI_EXEC_r;
    } else if (!strcmp("s", key)) {
      execute = HIKARI_EXEC_s;
    } else if (!strcmp("t", key)) {
      execute = HIKARI_EXEC_t;
    } else if (!strcmp("u", key)) {
      execute = HIKARI_EXEC_u;
    } else if (!strcmp("v", key)) {
      execute = HIKARI_EXEC_v;
    } else if (!strcmp("w", key)) {
      execute = HIKARI_EXEC_w;
    } else if (!strcmp("x", key)) {
      execute = HIKARI_EXEC_x;
    } else if (!strcmp("y", key)) {
      execute = HIKARI_EXEC_y;
    } else if (!strcmp("z", key)) {
      execute = HIKARI_EXEC_z;
    }

    if (execute != NULL) {
      execute->command = copy_in_config_string(cur);
      execute = NULL;
    } else {
      fprintf(stderr, "config error: invalid exec register %s\n", key);
      success = false;
      break;
    }
  }
  ucl_object_iterate_free(it);

  if (!success) {
    for (int i = 0; i < HIKARI_NR_OF_MARKS; i++) {
      hikari_free(execs[i].command);
    }
  }

  return success;
}

static bool
parse_autoconf(
    struct hikari_configuration *configuration, const ucl_object_t *obj)
{
  ucl_object_iter_t it = ucl_object_iterate_new(obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    struct hikari_view_autoconf *autoconf =
        hikari_malloc(sizeof(struct hikari_view_autoconf));

    const char *key = ucl_object_key(cur);
    size_t keylen = strlen(key);

    autoconf->app_id = hikari_malloc(keylen + 1);
    strcpy(autoconf->app_id, key);

    const ucl_object_t *group = ucl_object_lookup(cur, "group");
    if (group != NULL) {
      autoconf->group_name = copy_in_config_string(group);
    } else {
      autoconf->group_name = NULL;
    }

    const ucl_object_t *sheet = ucl_object_lookup(cur, "sheet");
    if (sheet != NULL) {
      autoconf->sheet_nr = ucl_object_toint(sheet);
    } else {
      autoconf->sheet_nr = -1;
    }

    const ucl_object_t *mark = ucl_object_lookup(cur, "mark");
    if (mark != NULL) {
      char mark_name = ucl_object_tostring(mark)[0];
      autoconf->mark = &marks[mark_name - 'a'];
    } else {
      autoconf->mark = NULL;
    }

    const ucl_object_t *position = ucl_object_lookup(cur, "position");
    if (position != NULL) {
      const ucl_object_t *x = ucl_object_lookup(position, "x");
      const ucl_object_t *y = ucl_object_lookup(position, "y");
      autoconf->position.x = ucl_object_toint(x);
      autoconf->position.y = ucl_object_toint(y);
    } else {
      autoconf->position.x = -1;
      autoconf->position.y = -1;
    }

    const ucl_object_t *focus = ucl_object_lookup(cur, "focus");
    if (focus != NULL) {
      autoconf->focus = ucl_object_toboolean(focus);
    } else {
      autoconf->focus = false;
    }

    wl_list_insert(&hikari_configuration.autoconfs, &autoconf->link);
  }

  ucl_object_iterate_free(it);

  return true;
}

static bool
parse_keycode(const char *str, uint32_t *keycode, uint32_t *modifiers)
{
  uint32_t mods = 0;
  uint32_t code = 0;

  size_t len = strlen(str);
  int pos;

  for (pos = 0; pos < len; pos++) {
    char c = str[pos];
    if (c == '-') {
      break;
    } else if (c == 'L') {
      mods |= WLR_MODIFIER_LOGO;
    } else if (c == 'S') {
      mods |= WLR_MODIFIER_SHIFT;
    } else if (c == 'A') {
      mods |= WLR_MODIFIER_ALT;
    } else if (c == 'C') {
      mods |= WLR_MODIFIER_CTRL;
    } else if (c == '5') {
      mods |= WLR_MODIFIER_MOD5;
    } else if (c == '0') {
      // do nothing
    } else {
      fprintf(stderr, "config error: could not parse keycode\n");
      return false;
    }
  }

  pos++;

  code = atoi(str + pos);

  *keycode = code;
  *modifiers = mods;
  return true;
}

static char *
lookup_action(const char *action, const ucl_object_t *actions)
{
  const ucl_object_t *obj = ucl_object_lookup(actions, action + 7);

  if (obj != NULL) {
    return copy_in_config_string(obj);
  } else {
    return NULL;
  }
}

static struct hikari_split *
lookup_layout(const char *layout, const ucl_object_t *layouts)
{
  const ucl_object_t *obj = ucl_object_lookup(layouts, layout + 7);

  if (obj != NULL) {
    return parse_split(obj);
  } else {
    return NULL;
  }
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
  bool success = ucl_object_tostring_safe(obj, &str);

  if (success) {
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
      char *command = lookup_action(str, actions);
      struct hikari_split *layout = lookup_layout(str, layouts);
      if (command != NULL) {
        *action = hikari_server_execute_command;
        *cleanup = cleanup_execute;
        *arg = command;
      } else if (layout != NULL) {
        *action = hikari_server_layout_sheet;
        *cleanup = cleanup_layout;
        *arg = layout;
      } else {
        fprintf(stderr, "config error: unknown action %s\n", str);
        assert(false);
        success = false;
      }
    }
  }

  return success;
}

static bool
parse_keyboard_bindings(struct hikari_configuration *configuration,
    const ucl_object_t *actions,
    const ucl_object_t *layouts,
    uint8_t *nbindings,
    struct hikari_keybinding **bindings,
    const ucl_object_t *bindings_keycode)
{
  int n[256] = { 0 };
  uint8_t mask = 0;
  const ucl_object_t *cur;
  bool success = true;

  ucl_object_iter_t it = ucl_object_iterate_new(bindings_keycode);
  while ((cur = ucl_object_iterate_safe(it, false)) != NULL) {
    const char *key = ucl_object_key(cur);

    if (!parse_modifier_mask(key, &mask)) {
      success = false;
      goto done;
    }
    // TODO check for overflow
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

  it = ucl_object_iterate_new(bindings_keycode);
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    const char *key = ucl_object_key(cur);

    parse_modifier_mask(key, &mask);

    struct hikari_keybinding *binding = &bindings[mask][n[mask]];

    if (!parse_keycode(key, &binding->keycode, &binding->modifiers) ||
        !parse_binding(cur,
            actions,
            layouts,
            &binding->action,
            &binding->cleanup,
            &binding->arg)) {
      success = false;
      // TODO cleanup
      goto done;
    }
    n[mask]++;
  }

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
  const ucl_object_t *keyboard_obj =
      ucl_object_lookup(bindings_obj, "keyboard");

  if (keyboard_obj != NULL) {
    parse_keyboard_bindings(configuration,
        actions,
        layouts,
        configuration->normal_mode.nkeybindings,
        configuration->normal_mode.keybindings,
        keyboard_obj);
  }

  const ucl_object_t *mouse_obj = ucl_object_lookup(bindings_obj, "mouse");

  if (mouse_obj != NULL) {
    parse_keyboard_bindings(configuration,
        actions,
        layouts,
        configuration->normal_mode.nmousebindings,
        configuration->normal_mode.mousebindings,
        mouse_obj);
  }

  return true;
}

static bool
parse_pointer_config(struct hikari_configuration *configuration,
    const ucl_object_t *pointer_config_obj)
{
  const char *key = ucl_object_key(pointer_config_obj);
  size_t keylen = strlen(key);

  struct hikari_pointer_config *pointer_config =
      hikari_malloc(sizeof(struct hikari_pointer_config));

  pointer_config->name = hikari_malloc(keylen + 1);
  strcpy(pointer_config->name, key);

  const ucl_object_t *accel_obj =
      ucl_object_lookup(pointer_config_obj, "accel");

  if (accel_obj != NULL) {
    pointer_config->accel = ucl_object_todouble(accel_obj);
  } else {
    pointer_config->accel = 0;
  }

  const ucl_object_t *scroll_button_obj =
      ucl_object_lookup(pointer_config_obj, "scroll-button");

  if (scroll_button_obj != NULL) {
    pointer_config->scroll_button = ucl_object_toint(scroll_button_obj);
  } else {
    pointer_config->scroll_button = 0;
  }

  const ucl_object_t *scroll_method_obj =
      ucl_object_lookup(pointer_config_obj, "scroll-method");

  if (scroll_method_obj != NULL) {
    const char *method = ucl_object_tostring(scroll_method_obj);

    if (!strcmp(method, "on-button-down")) {
      pointer_config->scroll_method = LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;
    } else {
      pointer_config->scroll_method = LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
    }

  } else {
    pointer_config->scroll_method = LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
  }

  wl_list_insert(&configuration->pointer_configs, &pointer_config->link);

  return true;
}

static bool
parse_pointers(
    struct hikari_configuration *configuration, const ucl_object_t *inputs_obj)
{
  ucl_object_iter_t it = ucl_object_iterate_new(inputs_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    parse_pointer_config(configuration, cur);
  }

  ucl_object_iterate_free(it);

  return true;
}

static bool
parse_inputs(
    struct hikari_configuration *configuration, const ucl_object_t *inputs_obj)
{
  const ucl_object_t *pointers_obj = ucl_object_lookup(inputs_obj, "pointers");

  if (pointers_obj != NULL) {
    parse_pointers(configuration, pointers_obj);
  }

  return true;
}

static bool
parse_background(struct hikari_configuration *configuration,
    const ucl_object_t *background_obj)
{
  const char *key = ucl_object_key(background_obj);
  char *background_path = copy_in_config_string(background_obj);
  struct hikari_background *background =
      hikari_malloc(sizeof(struct hikari_background));

  if (background_path != NULL) {
    hikari_background_init(background, key, background_path);

    wl_list_insert(&configuration->backgrounds, &background->link);
    return true;
  } else {
    return false;
  }
}

static bool
parse_backgrounds(struct hikari_configuration *configuration,
    const ucl_object_t *backgrounds_obj)
{
  bool success = true;
  ucl_object_iter_t it = ucl_object_iterate_new(backgrounds_obj);

  const ucl_object_t *cur;
  while ((cur = ucl_object_iterate_safe(it, true)) != NULL) {
    if (!parse_background(configuration, cur)) {
      success = false;
    }
  }
  ucl_object_iterate_free(it);

  return success;
}

static void
parse_border(
    struct hikari_configuration *configuration, const ucl_object_t *border_obj)
{
  int64_t border = 1;

  if (!ucl_object_toint_safe(border_obj, &border)) {
    assert(false);
  }

  configuration->border = border;
}

static void
parse_gap(
    struct hikari_configuration *configuration, const ucl_object_t *gap_obj)
{
  int64_t gap = 5;

  if (!ucl_object_toint_safe(gap_obj, &gap)) {
    assert(false);
  }

  configuration->gap = gap;
}

static void
parse_font(
    struct hikari_configuration *configuration, const ucl_object_t *font_obj)
{
  const char *font = ucl_object_tostring(font_obj);

  hikari_font_init(&configuration->font, font);
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

static void
parse_configuration(struct hikari_configuration *configuration)
{
  struct ucl_parser *parser = ucl_parser_new(0);
  char *config_path = get_config_path();
  bool success = true;

  ucl_parser_add_file(parser, config_path);
  ucl_object_t *obj = ucl_parser_get_object(parser);

  const ucl_object_t *colorscheme = ucl_object_lookup(obj, "colorscheme");
  const ucl_object_t *actions = ucl_object_lookup(obj, "actions");
  const ucl_object_t *layouts = ucl_object_lookup(obj, "layouts");
  const ucl_object_t *autoconf = ucl_object_lookup(obj, "autoconf");
  const ucl_object_t *execute = ucl_object_lookup(obj, "execute");
  const ucl_object_t *bindings = ucl_object_lookup(obj, "bindings");
  const ucl_object_t *backgrounds = ucl_object_lookup(obj, "backgrounds");
  const ucl_object_t *font = ucl_object_lookup(obj, "font");
  const ucl_object_t *border = ucl_object_lookup(obj, "border");
  const ucl_object_t *gap = ucl_object_lookup(obj, "gap");
  const ucl_object_t *inputs = ucl_object_lookup(obj, "inputs");

  if (colorscheme != NULL) {
    parse_colorscheme(configuration, colorscheme);
  } else {
    hikari_color_convert(configuration->clear, 0x282C34);
    hikari_color_convert(configuration->foreground, 0x000000);
    hikari_color_convert(configuration->indicator_selected, 0xE6DB74);
    hikari_color_convert(configuration->indicator_grouped, 0xFD971F);
    hikari_color_convert(configuration->indicator_first, 0xB8E673);
    hikari_color_convert(configuration->indicator_conflict, 0xEF5939);
    hikari_color_convert(configuration->indicator_insert, 0x66D9EF);
    hikari_color_convert(configuration->border_active, 0xFFFFFF);
    hikari_color_convert(configuration->border_inactive, 0x465457);
  }

  if (autoconf != NULL && !parse_autoconf(configuration, autoconf)) {
    success = false;
    goto done;
  }

  if (execute != NULL && !parse_execute(configuration, execute)) {
    success = false;
    goto done;
  }

  if (bindings != NULL &&
      !parse_bindings(configuration, actions, layouts, bindings)) {
    success = false;
    goto done;
  }

  if (backgrounds != NULL && !parse_backgrounds(configuration, backgrounds)) {
    success = false;
    goto done;
  }

  if (inputs != NULL && !parse_inputs(configuration, inputs)) {
    success = false;
    goto done;
  }

  if (font != NULL) {
    parse_font(configuration, font);
  }

  if (border != NULL) {
    parse_border(configuration, border);
  } else {
    configuration->border = 1;
  }

  if (gap != NULL) {
    parse_gap(configuration, gap);
  } else {
    configuration->gap = 5;
  }

done:

  ucl_object_unref(obj);
  ucl_parser_free(parser);

  free(config_path);

  if (!success) {
    exit(EXIT_FAILURE);
  }
}

void
hikari_configuration_init(struct hikari_configuration *configuration)
{
  wl_list_init(&configuration->autoconfs);
  wl_list_init(&configuration->backgrounds);
  wl_list_init(&configuration->pointer_configs);

  return parse_configuration(configuration);
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
