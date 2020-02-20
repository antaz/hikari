#include <hikari/indicator.h>

#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>

#include <hikari/configuration.h>
#include <hikari/mark.h>
#include <hikari/render_data.h>
#include <hikari/sheet.h>
#include <hikari/view.h>

void
hikari_indicator_init(struct hikari_indicator *indicator, float color[static 4])
{
  int width;
  hikari_font_metrics(
      &hikari_configuration->font, "", &width, &indicator->bar_height);
  indicator->bar_height += 8;

  int offset = 5;
  hikari_indicator_bar_init(&indicator->title, indicator, offset);
  offset += indicator->bar_height + 5;
  hikari_indicator_bar_init(&indicator->sheet, indicator, offset);
  offset += indicator->bar_height + 5;
  hikari_indicator_bar_init(&indicator->group, indicator, offset);
  offset += indicator->bar_height + 5;
  hikari_indicator_bar_init(&indicator->mark, indicator, offset);
}

void
hikari_indicator_fini(struct hikari_indicator *indicator)
{
  hikari_indicator_bar_fini(&indicator->title);
  hikari_indicator_bar_fini(&indicator->sheet);
  hikari_indicator_bar_fini(&indicator->group);
  hikari_indicator_bar_fini(&indicator->mark);
}

void
hikari_indicator_update(struct hikari_indicator *indicator,
    struct hikari_view *view,
    float background[static 4])
{
  assert(view != NULL);

  struct wlr_box *geometry = hikari_view_geometry(view);
  struct hikari_output *output = view->output;

  hikari_indicator_update_title(
      indicator, geometry, output, view->title, background);

  hikari_indicator_update_sheet(indicator,
      geometry,
      output,
      view->sheet,
      background,
      hikari_view_is_iconified(view),
      hikari_view_is_floating(view));

  if (view->sheet->group != view->group) {
    hikari_indicator_update_group(
        indicator, geometry, output, view->group->name, background);
  } else {
    hikari_indicator_update_group(indicator, geometry, output, "", background);
  }

  if (view->mark != NULL) {
    hikari_indicator_update_mark(
        indicator, geometry, output, view->mark->name, background);
  } else {
    hikari_indicator_update_mark(indicator, geometry, output, "", background);
  }
}

void
hikari_indicator_render(
    struct hikari_indicator *indicator, struct hikari_render_data *render_data)
{
  struct wlr_box *border_geometry = render_data->geometry;
  struct wlr_box geometry = *border_geometry;

  render_data->geometry = &geometry;

  geometry.x += 5;

  struct hikari_indicator_bar *title_bar = &indicator->title;
  geometry.y += 5;
  hikari_indicator_bar_render(title_bar, render_data);

  struct hikari_indicator_bar *sheet_bar = &indicator->sheet;
  geometry.y += indicator->bar_height + 5;
  hikari_indicator_bar_render(sheet_bar, render_data);

  struct hikari_indicator_bar *group_bar = &indicator->group;
  geometry.y += indicator->bar_height + 5;
  hikari_indicator_bar_render(group_bar, render_data);

  struct hikari_indicator_bar *mark_bar = &indicator->mark;
  geometry.y += indicator->bar_height + 5;
  hikari_indicator_bar_render(mark_bar, render_data);

  render_data->geometry = border_geometry;
}

void
hikari_indicator_update_sheet(struct hikari_indicator *indicator,
    struct wlr_box *view_geometry,
    struct hikari_output *output,
    struct hikari_sheet *sheet,
    float background[static 4],
    bool iconified,
    bool floating)
{
  char *output_name = output->output->name;
  char *text = hikari_malloc(strlen(output_name) + 12);
  int i = 0;

  if (floating) {
    text[i++] = '~';
  }

  if (iconified) {
    text[i++] = '[';
    text[i++] = sheet->group->name[0];
    text[i++] = ']';
  } else {
    text[i++] = sheet->group->name[0];
  }

  if (sheet->workspace->sheet != sheet) {
    text[i++] = ' ';
    text[i++] = '@';
    text[i++] = ' ';
    text[i++] = sheet->workspace->sheet->group->name[0];
  }

  text[i++] = ' ';
  text[i++] = '-';
  text[i++] = ' ';

  strcpy(&text[i], output_name);

  hikari_indicator_bar_update(
      &indicator->sheet, view_geometry, output, text, background);

  hikari_free(text);
}

void
hikari_indicator_damage(
    struct hikari_indicator *indicator, struct hikari_view *view)
{
  assert(indicator != NULL);
  assert(view != NULL);

  struct wlr_box *geometry = hikari_view_border_geometry(view);
  struct hikari_output *output = view->output;

  hikari_indicator_bar_damage(&indicator->title, geometry, output);
  hikari_indicator_bar_damage(&indicator->sheet, geometry, output);
  hikari_indicator_bar_damage(&indicator->group, geometry, output);
  hikari_indicator_bar_damage(&indicator->mark, geometry, output);
}
