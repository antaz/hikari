#include <hikari/mark.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <hikari/memory.h>
#include <hikari/view.h>

struct hikari_mark hikari_marks[HIKARI_NR_OF_MARKS];

struct hikari_mark *HIKARI_MARK_a = &hikari_marks[0];
struct hikari_mark *HIKARI_MARK_b = &hikari_marks[1];
struct hikari_mark *HIKARI_MARK_c = &hikari_marks[2];
struct hikari_mark *HIKARI_MARK_d = &hikari_marks[3];
struct hikari_mark *HIKARI_MARK_e = &hikari_marks[4];
struct hikari_mark *HIKARI_MARK_f = &hikari_marks[5];
struct hikari_mark *HIKARI_MARK_g = &hikari_marks[6];
struct hikari_mark *HIKARI_MARK_h = &hikari_marks[7];
struct hikari_mark *HIKARI_MARK_i = &hikari_marks[8];
struct hikari_mark *HIKARI_MARK_j = &hikari_marks[9];
struct hikari_mark *HIKARI_MARK_k = &hikari_marks[10];
struct hikari_mark *HIKARI_MARK_l = &hikari_marks[11];
struct hikari_mark *HIKARI_MARK_m = &hikari_marks[12];
struct hikari_mark *HIKARI_MARK_n = &hikari_marks[13];
struct hikari_mark *HIKARI_MARK_o = &hikari_marks[14];
struct hikari_mark *HIKARI_MARK_p = &hikari_marks[15];
struct hikari_mark *HIKARI_MARK_q = &hikari_marks[16];
struct hikari_mark *HIKARI_MARK_r = &hikari_marks[17];
struct hikari_mark *HIKARI_MARK_s = &hikari_marks[18];
struct hikari_mark *HIKARI_MARK_t = &hikari_marks[19];
struct hikari_mark *HIKARI_MARK_u = &hikari_marks[20];
struct hikari_mark *HIKARI_MARK_v = &hikari_marks[21];
struct hikari_mark *HIKARI_MARK_w = &hikari_marks[22];
struct hikari_mark *HIKARI_MARK_x = &hikari_marks[23];
struct hikari_mark *HIKARI_MARK_y = &hikari_marks[24];
struct hikari_mark *HIKARI_MARK_z = &hikari_marks[25];

void
hikari_marks_init(void)
{
  char *name;
  for (int i = 0; i < HIKARI_NR_OF_MARKS; i++) {
    name = hikari_malloc(2);
    snprintf(name, 2, "%c", 'a' + i);
    hikari_marks[i].name = name;
    hikari_marks[i].nr = i;
    hikari_marks[i].view = NULL;
  }
}

void
hikari_marks_fini(void)
{
  for (int i = 0; i < HIKARI_NR_OF_MARKS; i++) {
    hikari_free(hikari_marks[i].name);
  }
}

void
hikari_mark_clear(struct hikari_mark *mark)
{
  assert(mark != NULL);

  if (mark->view != NULL) {
    mark->view->mark = NULL;
    mark->view = NULL;
  }
}

void
hikari_mark_set(struct hikari_mark *mark, struct hikari_view *view)
{
  assert(mark != NULL);
  assert(view != NULL);

  if (view->mark != NULL) {
    hikari_mark_clear(view->mark);
  }

  hikari_mark_clear(mark);

  mark->view = view;
  view->mark = mark;
}

bool
hikari_mark_get(char reg, struct hikari_mark **mark)
{

  if (reg >= 'a' && reg <= 'z') {
    uint32_t nr = reg - 'a';
    *mark = &hikari_marks[nr];

    return true;
  }

  return false;
}
