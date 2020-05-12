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
  int bar_height = hikari_configuration->font.height;

  int offset = 5;
  hikari_indicator_bar_init(&indicator->title, indicator, offset);
  offset += bar_height + 5;
  hikari_indicator_bar_init(&indicator->sheet, indicator, offset);
  offset += bar_height + 5;
  hikari_indicator_bar_init(&indicator->group, indicator, offset);
  offset += bar_height + 5;
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

  struct hikari_output *output = view->output;

  hikari_indicator_update_title(indicator, output, view->title, background);

  hikari_indicator_update_sheet(
      indicator, output, view->sheet, view->flags, background);

  hikari_indicator_update_group(
      indicator, output, view->group->name, background);

  if (view->mark != NULL) {
    hikari_indicator_update_mark(
        indicator, output, view->mark->name, background);
  } else {
    hikari_indicator_update_mark(indicator, output, "", background);
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

  int bar_height = hikari_configuration->font.height;

  struct hikari_indicator_bar *sheet_bar = &indicator->sheet;
  geometry.y += bar_height + 5;
  hikari_indicator_bar_render(sheet_bar, render_data);

  struct hikari_indicator_bar *group_bar = &indicator->group;
  geometry.y += bar_height + 5;
  hikari_indicator_bar_render(group_bar, render_data);

  struct hikari_indicator_bar *mark_bar = &indicator->mark;
  geometry.y += bar_height + 5;
  hikari_indicator_bar_render(mark_bar, render_data);

  render_data->geometry = border_geometry;
}

static char
sheet_name(struct hikari_sheet *sheet)
{
  return sheet->nr + 48;
}

void
hikari_indicator_update_sheet(struct hikari_indicator *indicator,
    struct hikari_output *output,
    struct hikari_sheet *sheet,
    unsigned long flags,
    float background[static 4])
{
  bool invisible = flags & hikari_view_invisible_flag;
  bool floating = flags & hikari_view_floating_flag;
  char *output_name = sheet->workspace->output->wlr_output->name;
  int i = 0;

  char *text = hikari_malloc(strlen(output_name) + 12);

  if (floating) {
    text[i++] = '~';
  }

  if (invisible) {
    text[i++] = '[';
    text[i++] = sheet_name(sheet);
    text[i++] = ']';
  } else {
    text[i++] = sheet_name(sheet);
  }

  if (sheet->workspace->sheet != sheet) {
    text[i++] = ' ';
    text[i++] = '@';
    text[i++] = ' ';
    text[i++] = sheet_name(sheet->workspace->sheet);
  }

  text[i++] = ' ';
  text[i++] = '-';
  text[i++] = ' ';

  strcpy(&text[i], output_name);

  hikari_indicator_bar_update(&indicator->sheet, output, text, background);

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

  hikari_indicator_bar_damage(&indicator->title, output, geometry);
  hikari_indicator_bar_damage(&indicator->sheet, output, geometry);
  hikari_indicator_bar_damage(&indicator->group, output, geometry);
  hikari_indicator_bar_damage(&indicator->mark, output, geometry);
}
