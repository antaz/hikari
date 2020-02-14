#include <hikari/exec.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <hikari/memory.h>
#include <hikari/view.h>

void
hikari_exec_init(struct hikari_exec *exec)
{
  exec->command = NULL;
}

void
hikari_exec_fini(struct hikari_exec *exec)
{
  hikari_free(exec->command);
}
