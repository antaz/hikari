#include <stdlib.h>
#include <unistd.h>

#include <wlr/util/log.h>

#include <hikari/server.h>

#include "version.h"

static char *
get_default_path(char *path)
{
  char *config_home = getenv("HOME");
  size_t len = strlen(config_home) + strlen(path);

  char *ret = malloc(len + 1);

  memset(ret, 0, len + 1);

  strcat(ret, config_home);
  strcat(ret, path);

  return ret;
}

static char *
get_default_autostart(void)
{
  return get_default_path("/.config/hikari/autostart");
}

static char *
get_default_config_path(void)
{
  return get_default_path("/.config/hikari/hikari.conf");
}

const char *usage = "Usage: hikari [options]\n"
                    "\n"
                    "Options: \n"
                    "  -a <executable> Specify an autostart executable.\n"
                    "  -c <config>     Specify a configuration file.\n"
                    "  -h              Show this message and quit.\n"
                    "  -v              Show version and quit.\n"
                    "\n";

int
main(int argc, char **argv)
{
#ifndef NDEBUG
  wlr_log_init(WLR_DEBUG, NULL);
#endif
  char *config_path = NULL;
  char *autostart = NULL;

  char flag;
  while ((flag = getopt(argc, argv, "vhc:a:")) != -1) {
    switch (flag) {
      case 'a':
        free(autostart);
        autostart = strdup(optarg);
        break;

      case 'c':
        free(config_path);
        config_path = strdup(optarg);
        break;

      case 'v':
        free(config_path);
        free(autostart);

        printf("%s\n", HIKARI_VERSION);
        return EXIT_SUCCESS;

      case 'h':
        free(config_path);
        free(autostart);

        printf("%s", usage);
        return EXIT_SUCCESS;

      case '?':
      default:
        free(config_path);
        free(autostart);

        printf("%s", usage);
        return EXIT_FAILURE;
    }
  }

  if (config_path == NULL) {
    config_path = get_default_config_path();
  }

  if (autostart == NULL) {
    autostart = get_default_autostart();
  }

  hikari_server_start(config_path, autostart);
  hikari_server_stop();

  return EXIT_SUCCESS;
}
