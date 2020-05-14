#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <wlr/util/log.h>

#include <hikari/server.h>

#include "version.h"

static char *
get_default_path(char *path)
{
  char *prefix = getenv("XDG_CONFIG_HOME");
  char *subdirectory;

  if (prefix == NULL) {
    prefix = getenv("HOME");
    subdirectory = "/.config/hikari/";
  } else {
    subdirectory = "/hikari/";
  }

  size_t len = strlen(prefix) + strlen(subdirectory) + strlen(path);

  char *ret = malloc(len + 1);

  strcpy(ret, prefix);
  strcat(ret, subdirectory);
  strcat(ret, path);

  return ret;
}

static char *
get_default_autostart(void)
{
  return get_default_path("autostart");
}

static char *
get_path(const char *path)
{
  char *ret = NULL;
  size_t len = strlen(path) + 1;
  struct stat s;

  if (stat(path, &s) == 0 && S_ISREG(s.st_mode)) {
    ret = malloc(len + 1);
    strcpy(ret, path);

    return ret;
  }

  return NULL;
}

#define STR(s) #s
#define DEFAULT_CONFIG(s) STR(s) "/etc/hikari/hikari.conf"
#define DEFAULT_CONFIG_FILE DEFAULT_CONFIG(HIKARI_ETC_PREFIX)

static const char *default_config_file = DEFAULT_CONFIG_FILE;

static char *
get_default_config_path(void)
{
  char *ret = get_default_path("hikari.conf");
  struct stat s;

  if (stat(ret, &s) == 0 && S_ISREG(s.st_mode)) {
    return ret;
  } else {
    free(ret);

    if ((ret = get_path(default_config_file)) != NULL) {
      return ret;
    }
  }

  return NULL;
}
#undef STR
#undef DEFAULT_CONFIG
#undef DEFAULT_CONFIG_FILE

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

    if (config_path == NULL) {
      free(autostart);
      fprintf(stderr,
          "could not load default configuration \"%s\"\n",
          default_config_file);
      exit(EXIT_FAILURE);
    }
  }

  if (autostart == NULL) {
    autostart = get_default_autostart();
  }

  hikari_server_start(config_path, autostart);
  hikari_server_stop();

  return EXIT_SUCCESS;
}
