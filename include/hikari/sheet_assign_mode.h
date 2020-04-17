#if !defined(HIKARI_SHEET_ASSIGN_MODE_H)
#define HIKARI_SHEET_ASSIGN_MODE_H

#include <hikari/input_buffer.h>
#include <hikari/mode.h>

struct hikari_sheet;
struct hikari_view;

struct hikari_sheet_assign_mode {
  struct hikari_mode mode;
  struct hikari_input_buffer input_buffer;
  struct hikari_sheet *sheet;
};

void
hikari_sheet_assign_mode_init(
    struct hikari_sheet_assign_mode *sheet_assign_mode);

void
hikari_sheet_assign_mode_enter(struct hikari_view *view);

#endif
