#include <stdlib.h>
#include <wlr/util/log.h>

#include <hikari/server.h>

int
main(int argc, char **argv)
{
#ifndef NDEBUG
  wlr_log_init(WLR_DEBUG, NULL);
#endif
  hikari_server_start();
  hikari_server_stop();

  return EXIT_SUCCESS;
}
