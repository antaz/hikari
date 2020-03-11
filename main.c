#include <stdlib.h>
#include <unistd.h>

#include <wlr/util/log.h>

#include <hikari/server.h>

#include "version.h"

const char *usage = "Usage: hikari [options]\n"
                    "\n"
                    "  -h  Show this message and quit.\n"
                    "  -v  Show version and quit.\n"
                    "\n";

int
main(int argc, char **argv)
{
#ifndef NDEBUG
  wlr_log_init(WLR_DEBUG, NULL);
#endif

  char flag;
  while ((flag = getopt(argc, argv, "vh")) != -1) {
    switch (flag) {
      case 'v':
        printf("%s\n", HIKARI_VERSION);
        exit(0);
        break;

      case 'h':
      case '?':
      default:
        printf("%s", usage);
        exit(0);
        break;
    }
  }

  hikari_server_start();
  hikari_server_stop();

  return EXIT_SUCCESS;
}
