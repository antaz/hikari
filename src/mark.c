#include <hikari/mark.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <hikari/memory.h>
#include <hikari/view.h>

struct hikari_mark marks[HIKARI_NR_OF_MARKS];

struct hikari_mark *HIKARI_MARK_a = &marks[0];
struct hikari_mark *HIKARI_MARK_b = &marks[1];
struct hikari_mark *HIKARI_MARK_c = &marks[2];
struct hikari_mark *HIKARI_MARK_d = &marks[3];
struct hikari_mark *HIKARI_MARK_e = &marks[4];
struct hikari_mark *HIKARI_MARK_f = &marks[5];
struct hikari_mark *HIKARI_MARK_g = &marks[6];
struct hikari_mark *HIKARI_MARK_h = &marks[7];
struct hikari_mark *HIKARI_MARK_i = &marks[8];
struct hikari_mark *HIKARI_MARK_j = &marks[9];
struct hikari_mark *HIKARI_MARK_k = &marks[10];
struct hikari_mark *HIKARI_MARK_l = &marks[11];
struct hikari_mark *HIKARI_MARK_m = &marks[12];
struct hikari_mark *HIKARI_MARK_n = &marks[13];
struct hikari_mark *HIKARI_MARK_o = &marks[14];
struct hikari_mark *HIKARI_MARK_p = &marks[15];
struct hikari_mark *HIKARI_MARK_q = &marks[16];
struct hikari_mark *HIKARI_MARK_r = &marks[17];
struct hikari_mark *HIKARI_MARK_s = &marks[18];
struct hikari_mark *HIKARI_MARK_t = &marks[19];
struct hikari_mark *HIKARI_MARK_u = &marks[20];
struct hikari_mark *HIKARI_MARK_v = &marks[21];
struct hikari_mark *HIKARI_MARK_w = &marks[22];
struct hikari_mark *HIKARI_MARK_x = &marks[23];
struct hikari_mark *HIKARI_MARK_y = &marks[24];
struct hikari_mark *HIKARI_MARK_z = &marks[25];

void
hikari_marks_init(void)
{
  char *name;
  for (int i = 0; i < HIKARI_NR_OF_MARKS; i++) {
    name = hikari_malloc(2);
    snprintf(name, 2, "%c", 'a' + i);
    marks[i].name = name;
    marks[i].view = NULL;
  }
}

void
hikari_marks_fini(void)
{
  for (int i = 0; i < HIKARI_NR_OF_MARKS; i++) {
    hikari_free(marks[i].name);
  }
}

void
hikari_mark_clear(struct hikari_mark *mark)
{
  assert(mark != NULL);

  if (mark->view != NULL) {
    mark->view->mark = NULL;
    mark->view = NULL;
    ;
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
