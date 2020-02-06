#if !defined(HIKARI_EXEC_H)
#define HIKARI_EXEC_H

struct hikari_exec {
  char *command;
};

#define HIKARI_NR_OF_EXECS 26

extern struct hikari_exec execs[HIKARI_NR_OF_EXECS];

extern struct hikari_exec *HIKARI_EXEC_a;
extern struct hikari_exec *HIKARI_EXEC_b;
extern struct hikari_exec *HIKARI_EXEC_c;
extern struct hikari_exec *HIKARI_EXEC_d;
extern struct hikari_exec *HIKARI_EXEC_e;
extern struct hikari_exec *HIKARI_EXEC_f;
extern struct hikari_exec *HIKARI_EXEC_g;
extern struct hikari_exec *HIKARI_EXEC_h;
extern struct hikari_exec *HIKARI_EXEC_i;
extern struct hikari_exec *HIKARI_EXEC_j;
extern struct hikari_exec *HIKARI_EXEC_k;
extern struct hikari_exec *HIKARI_EXEC_l;
extern struct hikari_exec *HIKARI_EXEC_m;
extern struct hikari_exec *HIKARI_EXEC_n;
extern struct hikari_exec *HIKARI_EXEC_o;
extern struct hikari_exec *HIKARI_EXEC_p;
extern struct hikari_exec *HIKARI_EXEC_q;
extern struct hikari_exec *HIKARI_EXEC_r;
extern struct hikari_exec *HIKARI_EXEC_s;
extern struct hikari_exec *HIKARI_EXEC_t;
extern struct hikari_exec *HIKARI_EXEC_u;
extern struct hikari_exec *HIKARI_EXEC_v;
extern struct hikari_exec *HIKARI_EXEC_w;
extern struct hikari_exec *HIKARI_EXEC_x;
extern struct hikari_exec *HIKARI_EXEC_y;
extern struct hikari_exec *HIKARI_EXEC_z;

void
hikari_execs_init(void);

void
hikari_execs_fini(void);

#endif
