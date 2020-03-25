#if !defined(HIKARI_MARK_H)
#define HIKARI_MARK_H

#include <stdbool.h>

struct hikari_view;

struct hikari_mark {
  char *name;
  int nr;
  struct hikari_view *view;
};

#define HIKARI_NR_OF_MARKS 26

extern struct hikari_mark hikari_marks[HIKARI_NR_OF_MARKS];

extern struct hikari_mark *HIKARI_MARK_a;
extern struct hikari_mark *HIKARI_MARK_b;
extern struct hikari_mark *HIKARI_MARK_c;
extern struct hikari_mark *HIKARI_MARK_d;
extern struct hikari_mark *HIKARI_MARK_e;
extern struct hikari_mark *HIKARI_MARK_f;
extern struct hikari_mark *HIKARI_MARK_g;
extern struct hikari_mark *HIKARI_MARK_h;
extern struct hikari_mark *HIKARI_MARK_i;
extern struct hikari_mark *HIKARI_MARK_j;
extern struct hikari_mark *HIKARI_MARK_k;
extern struct hikari_mark *HIKARI_MARK_l;
extern struct hikari_mark *HIKARI_MARK_m;
extern struct hikari_mark *HIKARI_MARK_n;
extern struct hikari_mark *HIKARI_MARK_o;
extern struct hikari_mark *HIKARI_MARK_p;
extern struct hikari_mark *HIKARI_MARK_q;
extern struct hikari_mark *HIKARI_MARK_r;
extern struct hikari_mark *HIKARI_MARK_s;
extern struct hikari_mark *HIKARI_MARK_t;
extern struct hikari_mark *HIKARI_MARK_u;
extern struct hikari_mark *HIKARI_MARK_v;
extern struct hikari_mark *HIKARI_MARK_w;
extern struct hikari_mark *HIKARI_MARK_x;
extern struct hikari_mark *HIKARI_MARK_y;
extern struct hikari_mark *HIKARI_MARK_z;

void
hikari_marks_init(void);

void
hikari_marks_fini(void);

void
hikari_mark_clear(struct hikari_mark *mark);

void
hikari_mark_set(struct hikari_mark *mark, struct hikari_view *view);

bool
hikari_mark_get(char reg, struct hikari_mark **mark);

#endif
