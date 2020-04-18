#include <hikari/layout_select_mode.h>

#include <stdbool.h>

#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

#include <hikari/configuration.h>
#include <hikari/indicator.h>
#include <hikari/indicator_frame.h>
#include <hikari/keyboard.h>
#include <hikari/layout_config.h>
#include <hikari/normal_mode.h>
#include <hikari/render_data.h>
#include <hikari/view.h>
#include <hikari/workspace.h>

static struct hikari_split *
lookup_layout(struct hikari_configuration *configuration,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  uint32_t keycode = event->keycode + 8;
  uint32_t codepoint = hikari_keyboard_get_codepoint(keyboard, keycode);

  struct hikari_split *split = NULL;
  if (codepoint != 0 && (codepoint >= 'a' && codepoint <= 'z')) {
    char layout_register = codepoint;
    struct hikari_layout_config *layout_config;
    wl_list_for_each (layout_config, &configuration->layout_configs, link) {
      if (layout_config->layout_register == layout_register) {
        split = layout_config->split;
      }
    }
  }

  return split;
}

static void
apply_layout(struct hikari_configuration *configuration,
    struct wlr_event_keyboard_key *event,
    struct hikari_keyboard *keyboard)
{
  struct hikari_split *split = lookup_layout(configuration, event, keyboard);

  hikari_server_enter_normal_mode(NULL);

  if (split != NULL) {
    struct hikari_workspace *workspace = hikari_server.workspace;

    hikari_workspace_apply_split(workspace, split);
  }
}

static void
key_handler(struct wl_listener *listener, void *data)
{
  struct hikari_keyboard *keyboard = wl_container_of(listener, keyboard, key);
  struct wlr_event_keyboard_key *event = data;

  if (event->state == WLR_KEY_PRESSED) {
    apply_layout(hikari_configuration, event, keyboard);
  }
}

static void
modifier_handler(struct wl_listener *listener, void *data)
{}

static void
button_handler(struct wl_listener *listener, void *data)
{}

static void
render(struct hikari_output *output, struct hikari_render_data *render_data)
{}

static void
cancel(void)
{}

static void
cursor_move(void)
{}

void
hikari_layout_select_mode_init(
    struct hikari_layout_select_mode *layout_select_mode)
{
  layout_select_mode->mode.key_handler = key_handler;
  layout_select_mode->mode.button_handler = button_handler;
  layout_select_mode->mode.modifier_handler = modifier_handler;
  layout_select_mode->mode.render = render;
  layout_select_mode->mode.cancel = cancel;
  layout_select_mode->mode.cursor_move = cursor_move;
}

void
hikari_layout_select_mode_enter(void)
{
  struct hikari_server *server = &hikari_server;

  server->mode = (struct hikari_mode *)&server->layout_select_mode;

  hikari_server_refresh_indication();
}
