#include <hikari/exec.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <hikari/memory.h>
#include <hikari/view.h>

struct hikari_exec execs[HIKARI_NR_OF_EXECS];

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
