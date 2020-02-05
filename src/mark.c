#include <hikari/mark.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <hikari/memory.h>
#include <hikari/view.h>

struct hikari_mark marks[HIKARI_NR_OF_MARKS];

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
