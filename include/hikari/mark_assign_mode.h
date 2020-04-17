#if !defined(HIKARI_MARK_ASSIGN_MODE_H)
#define HIKARI_MARK_ASSIGN_MODE_H

#include <hikari/indicator.h>
#include <hikari/mode.h>

struct hikari_mark;
struct hikari_view;

struct hikari_mark_assign_mode {
  struct hikari_mode mode;
  struct hikari_mark *pending_mark;
  struct hikari_indicator indicator;
};

void
hikari_mark_assign_mode_init(struct hikari_mark_assign_mode *mark_assign_mode);

void
hikari_mark_assign_mode_fini(struct hikari_mark_assign_mode *mark_assign_mode);

void
hikari_mark_assign_mode_enter(struct hikari_view *view);

#endif
