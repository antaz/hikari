#include <hikari/exec.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <hikari/memory.h>
#include <hikari/view.h>

struct hikari_exec execs[HIKARI_NR_OF_EXECS];

struct hikari_exec *HIKARI_EXEC_a = &execs[0];
struct hikari_exec *HIKARI_EXEC_b = &execs[1];
struct hikari_exec *HIKARI_EXEC_c = &execs[2];
struct hikari_exec *HIKARI_EXEC_d = &execs[3];
struct hikari_exec *HIKARI_EXEC_e = &execs[4];
struct hikari_exec *HIKARI_EXEC_f = &execs[5];
struct hikari_exec *HIKARI_EXEC_g = &execs[6];
struct hikari_exec *HIKARI_EXEC_h = &execs[7];
struct hikari_exec *HIKARI_EXEC_i = &execs[8];
struct hikari_exec *HIKARI_EXEC_j = &execs[9];
struct hikari_exec *HIKARI_EXEC_k = &execs[10];
struct hikari_exec *HIKARI_EXEC_l = &execs[11];
struct hikari_exec *HIKARI_EXEC_m = &execs[12];
struct hikari_exec *HIKARI_EXEC_n = &execs[13];
struct hikari_exec *HIKARI_EXEC_o = &execs[14];
struct hikari_exec *HIKARI_EXEC_p = &execs[15];
struct hikari_exec *HIKARI_EXEC_q = &execs[16];
struct hikari_exec *HIKARI_EXEC_r = &execs[17];
struct hikari_exec *HIKARI_EXEC_s = &execs[18];
struct hikari_exec *HIKARI_EXEC_t = &execs[19];
struct hikari_exec *HIKARI_EXEC_u = &execs[20];
struct hikari_exec *HIKARI_EXEC_v = &execs[21];
struct hikari_exec *HIKARI_EXEC_w = &execs[22];
struct hikari_exec *HIKARI_EXEC_x = &execs[23];
struct hikari_exec *HIKARI_EXEC_y = &execs[24];
struct hikari_exec *HIKARI_EXEC_z = &execs[25];

void
hikari_execs_init(void)
{
  /* for (int i = 0; i < HIKARI_NR_OF_EXECS; i++) { */
  /*   execs[i].command = NULL; */
  /* } */
}

void
hikari_execs_fini(void)
{
  for (int i = 0; i < HIKARI_NR_OF_EXECS; i++) {
    hikari_free(execs[i].command);
  }
}
