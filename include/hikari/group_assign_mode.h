#if !defined(HIKARI_GROUP_ASSIGN_MODE_H)
#define HIKARI_GROUP_ASSIGN_MODE_H

#include <hikari/input_buffer.h>
#include <hikari/mode.h>

struct hikari_group;
struct hikari_view;

struct hikari_group_assign_mode {
  struct hikari_mode mode;
  struct hikari_input_buffer input_buffer;
  struct hikari_completion *completion;
  struct hikari_group *group;
};

void
hikari_group_assign_mode_init(
    struct hikari_group_assign_mode *group_assign_mode);

void
hikari_group_assign_mode_enter(struct hikari_view *view);

#endif
