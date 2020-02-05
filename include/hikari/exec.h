#if !defined(HIKARI_EXEC_H)
#define HIKARI_EXEC_H

static const int HIKARI_NR_OF_EXECS = 26;

struct hikari_exec {
  char *command;
};

extern struct hikari_exec execs[HIKARI_NR_OF_EXECS];

static struct hikari_exec *HIKARI_EXEC_a = &execs[0];
static struct hikari_exec *HIKARI_EXEC_b = &execs[1];
static struct hikari_exec *HIKARI_EXEC_c = &execs[2];
static struct hikari_exec *HIKARI_EXEC_d = &execs[3];
static struct hikari_exec *HIKARI_EXEC_e = &execs[4];
static struct hikari_exec *HIKARI_EXEC_f = &execs[5];
static struct hikari_exec *HIKARI_EXEC_g = &execs[6];
static struct hikari_exec *HIKARI_EXEC_h = &execs[7];
static struct hikari_exec *HIKARI_EXEC_i = &execs[8];
static struct hikari_exec *HIKARI_EXEC_j = &execs[9];
static struct hikari_exec *HIKARI_EXEC_k = &execs[10];
static struct hikari_exec *HIKARI_EXEC_l = &execs[11];
static struct hikari_exec *HIKARI_EXEC_m = &execs[12];
static struct hikari_exec *HIKARI_EXEC_n = &execs[13];
static struct hikari_exec *HIKARI_EXEC_o = &execs[14];
static struct hikari_exec *HIKARI_EXEC_p = &execs[15];
static struct hikari_exec *HIKARI_EXEC_q = &execs[16];
static struct hikari_exec *HIKARI_EXEC_r = &execs[17];
static struct hikari_exec *HIKARI_EXEC_s = &execs[18];
static struct hikari_exec *HIKARI_EXEC_t = &execs[19];
static struct hikari_exec *HIKARI_EXEC_u = &execs[20];
static struct hikari_exec *HIKARI_EXEC_v = &execs[21];
static struct hikari_exec *HIKARI_EXEC_w = &execs[22];
static struct hikari_exec *HIKARI_EXEC_x = &execs[23];
static struct hikari_exec *HIKARI_EXEC_y = &execs[24];
static struct hikari_exec *HIKARI_EXEC_z = &execs[25];

void
hikari_execs_init(void);

void
hikari_execs_fini(void);

#endif
