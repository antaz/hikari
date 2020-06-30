#if !defined(HIKARI_CONFIGURATION_H)
#define HIKARI_CONFIGURATION_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#include <hikari/exec.h>
#include <hikari/font.h>
#include <hikari/mark.h>

struct hikari_group;
struct hikari_binding;
struct hikari_sheet;
struct hikari_view;
struct hikari_pointer_config;

struct hikari_configuration {
  float clear[4];
  float foreground[4];
  float indicator_selected[4];
  float indicator_grouped[4];
  float indicator_first[4];
  float indicator_conflict[4];
  float indicator_insert[4];
  float border_active[4];
  float border_inactive[4];

  struct hikari_font font;

  int border;
  int gap;
  int step;

  struct hikari_exec execs[HIKARI_NR_OF_EXECS];

  struct wl_list view_configs;
  struct wl_list output_configs;
  struct wl_list pointer_configs;
  struct wl_list keyboard_configs;
  struct wl_list layout_configs;
  struct wl_list action_configs;
  struct wl_list keyboard_binding_configs;
  struct wl_list mouse_binding_configs;
  struct wl_list switch_configs;
};

extern struct hikari_configuration *hikari_configuration;

void
hikari_configuration_init(struct hikari_configuration *configuration);

void
hikari_configuration_fini(struct hikari_configuration *configuration);

bool
hikari_configuration_load(
    struct hikari_configuration *configuration, char *config_path);

bool
hikari_configuration_reload(char *config_path);

struct hikari_view_config *
hikari_configuration_resolve_view_config(
    struct hikari_configuration *configuration, const char *app_id);

struct hikari_output_config *
hikari_configuration_resolve_output_config(
    struct hikari_configuration *configuration, const char *output_name);

struct hikari_pointer_config *
hikari_configuration_resolve_pointer_config(
    struct hikari_configuration *configuration, const char *pointer_name);

struct hikari_keyboard_config *
hikari_configuration_resolve_keyboard_config(
    struct hikari_configuration *configuration, const char *keyboard_name);

struct hikari_switch_config *
hikari_configuration_resolve_switch_config(
    struct hikari_configuration *configuration, const char *switch_name);

struct hikari_split *
hikari_configuration_lookup_layout(
    struct hikari_configuration *configuration, char layout_register);

#endif
