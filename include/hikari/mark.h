#if !defined(HIKARI_MARK_H)
#define HIKARI_MARK_H

struct hikari_view;

struct hikari_mark {
  char *name;
  struct hikari_view *view;
};

static const int HIKARI_NR_OF_MARKS = 26;

extern struct hikari_mark marks[HIKARI_NR_OF_MARKS];

static struct hikari_mark *const HIKARI_MARK_a = &marks[0];
static struct hikari_mark *const HIKARI_MARK_b = &marks[1];
static struct hikari_mark *const HIKARI_MARK_c = &marks[2];
static struct hikari_mark *const HIKARI_MARK_d = &marks[3];
static struct hikari_mark *const HIKARI_MARK_e = &marks[4];
static struct hikari_mark *const HIKARI_MARK_f = &marks[5];
static struct hikari_mark *const HIKARI_MARK_g = &marks[6];
static struct hikari_mark *const HIKARI_MARK_h = &marks[7];
static struct hikari_mark *const HIKARI_MARK_i = &marks[8];
static struct hikari_mark *const HIKARI_MARK_j = &marks[9];
static struct hikari_mark *const HIKARI_MARK_k = &marks[10];
static struct hikari_mark *const HIKARI_MARK_l = &marks[11];
static struct hikari_mark *const HIKARI_MARK_m = &marks[12];
static struct hikari_mark *const HIKARI_MARK_n = &marks[13];
static struct hikari_mark *const HIKARI_MARK_o = &marks[14];
static struct hikari_mark *const HIKARI_MARK_p = &marks[15];
static struct hikari_mark *const HIKARI_MARK_q = &marks[16];
static struct hikari_mark *const HIKARI_MARK_r = &marks[17];
static struct hikari_mark *const HIKARI_MARK_s = &marks[18];
static struct hikari_mark *const HIKARI_MARK_t = &marks[19];
static struct hikari_mark *const HIKARI_MARK_u = &marks[20];
static struct hikari_mark *const HIKARI_MARK_v = &marks[21];
static struct hikari_mark *const HIKARI_MARK_w = &marks[22];
static struct hikari_mark *const HIKARI_MARK_x = &marks[23];
static struct hikari_mark *const HIKARI_MARK_y = &marks[24];
static struct hikari_mark *const HIKARI_MARK_z = &marks[25];

void
hikari_marks_init(void);

void
hikari_marks_fini(void);

void
hikari_mark_clear(struct hikari_mark *mark);

void
hikari_mark_set(struct hikari_mark *mark, struct hikari_view *view);

#endif
