#if !defined(HIKARI_CONFIGURATION_H)
#define HIKARI_CONFIGURATION_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#include <hikari/font.h>

struct hikari_group;
struct hikari_keybinding;
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

  struct {
    uint8_t nkeybindings[256];
    struct hikari_keybinding *keybindings[256];

    uint8_t nmousebindings[256];
    struct hikari_keybinding *mousebindings[256];
  } normal_mode;

  struct wl_list autoconfs;
  struct wl_list backgrounds;
  struct wl_list pointer_configs;
};

extern struct hikari_configuration hikari_configuration;

void
hikari_configuration_init(struct hikari_configuration *configuration);

void
hikari_configuration_fini(struct hikari_configuration *configuration);

bool
hikari_configuration_load(struct hikari_configuration *configuration);

void
hikari_configuration_resolve_view_autoconf(
    struct hikari_configuration *configuration,
    const char *app_id,
    struct hikari_view *view,
    struct hikari_sheet **sheet,
    struct hikari_group **group,
    int *x,
    int *y,
    bool *focus);

char *
hikari_configuration_resolve_background(
    struct hikari_configuration *configuration, const char *output_name);

struct hikari_pointer_config *
hikari_configuration_resolve_pointer_config(
    struct hikari_configuration *configuration, const char *pointer_name);

#endif
