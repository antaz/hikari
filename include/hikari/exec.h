#if !defined(HIKARI_EXEC_H)
#define HIKARI_EXEC_H

struct hikari_exec {
  char *command;
};

#define HIKARI_NR_OF_EXECS 26

#define HIKARI_EXEC_a &hikari_configuration->execs[0]
#define HIKARI_EXEC_b &hikari_configuration->execs[1]
#define HIKARI_EXEC_c &hikari_configuration->execs[2]
#define HIKARI_EXEC_d &hikari_configuration->execs[3]
#define HIKARI_EXEC_e &hikari_configuration->execs[4]
#define HIKARI_EXEC_f &hikari_configuration->execs[5]
#define HIKARI_EXEC_g &hikari_configuration->execs[6]
#define HIKARI_EXEC_h &hikari_configuration->execs[7]
#define HIKARI_EXEC_i &hikari_configuration->execs[8]
#define HIKARI_EXEC_j &hikari_configuration->execs[9]
#define HIKARI_EXEC_k &hikari_configuration->execs[10]
#define HIKARI_EXEC_l &hikari_configuration->execs[11]
#define HIKARI_EXEC_m &hikari_configuration->execs[12]
#define HIKARI_EXEC_n &hikari_configuration->execs[13]
#define HIKARI_EXEC_o &hikari_configuration->execs[14]
#define HIKARI_EXEC_p &hikari_configuration->execs[15]
#define HIKARI_EXEC_q &hikari_configuration->execs[16]
#define HIKARI_EXEC_r &hikari_configuration->execs[17]
#define HIKARI_EXEC_s &hikari_configuration->execs[18]
#define HIKARI_EXEC_t &hikari_configuration->execs[19]
#define HIKARI_EXEC_u &hikari_configuration->execs[20]
#define HIKARI_EXEC_v &hikari_configuration->execs[21]
#define HIKARI_EXEC_w &hikari_configuration->execs[22]
#define HIKARI_EXEC_x &hikari_configuration->execs[23]
#define HIKARI_EXEC_y &hikari_configuration->execs[24]
#define HIKARI_EXEC_z &hikari_configuration->execs[25]

void
hikari_exec_init(struct hikari_exec *exec);

void
hikari_exec_fini(struct hikari_exec *exec);

#endif
