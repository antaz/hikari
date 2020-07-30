#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NDEBUG
#include <wlr/util/log.h>
#endif

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
get_user_autostart(void)
{
  return get_default_path("autostart");
}

static char *
get_user_config_path(void)
{
  return get_default_path("hikari.conf");
}

#define STR(s) #s
#define DEFAULT_CONFIG(s) STR(s) "/etc/hikari/hikari.conf"
#define DEFAULT_CONFIG_FILE DEFAULT_CONFIG(HIKARI_ETC_PREFIX)

static char *
get_default_config_path(void)
{
  return strdup(DEFAULT_CONFIG_FILE);
}

#undef STR
#undef DEFAULT_CONFIG
#undef DEFAULT_CONFIG_FILE

static bool
check_perms(char *path, int mode)
{
  struct stat s;
  return stat(path, &s) == 0 && S_ISREG(s.st_mode) && !access(path, mode);
}

static char *
check_path(char *path, int mode)
{
  char *check = path;

  if (!check_perms(check, mode)) {
    free(path);
    check = NULL;
  }

  return check;
}

static char *
get_config_path(char *path)
{
  char *config;

  if (path != NULL) {
    char *option_config = check_path(path, R_OK);

    config = option_config;
  } else {
    char *user_config = check_path(get_user_config_path(), R_OK);

    if (user_config == NULL) {
      char *default_config = check_path(get_default_config_path(), R_OK);

      config = default_config;
    } else {
      config = user_config;
    }
  }

  return config;
}

static char *
get_autostart(char *path)
{
  char *autostart;
  int mode = R_OK | X_OK;

  if (path != NULL) {
    char *option_autostart = check_path(path, mode);

    autostart = option_autostart;
  } else {
    char *user_autostart = check_path(get_user_autostart(), mode);

    autostart = user_autostart;
  }

  return autostart;
}

const char *usage = "Usage: hikari [options]\n"
                    "\n"
                    "Options: \n"
                    "  -a <executable> Specify an autostart executable.\n"
                    "  -c <config>     Specify a configuration file.\n"
                    "  -h              Show this message and quit.\n"
                    "  -v              Show version and quit.\n"
                    "\n";

struct options {
  char *config_path;
  char *autostart;
};

static void
parse_options(int argc, char **argv, struct options *options)
{
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
        exit(EXIT_SUCCESS);
        break;

      case 'h':
        free(config_path);
        free(autostart);

        printf("%s", usage);
        exit(EXIT_SUCCESS);
        break;

      case '?':
      default:
        free(config_path);
        free(autostart);

        printf("%s", usage);
        exit(EXIT_FAILURE);
        break;
    }
  }

  options->config_path = get_config_path(config_path);
  options->autostart = get_autostart(autostart);
}

int
main(int argc, char **argv)
{
#ifndef NDEBUG
  wlr_log_init(WLR_DEBUG, NULL);
#endif
  struct options options;
  parse_options(argc, argv, &options);

  if (options.config_path == NULL) {
    free(options.autostart);

    fprintf(stderr, "could not load configuration\n");

    return EXIT_FAILURE;
  } else {
    hikari_server_prepare_privileged();

    assert(geteuid() != 0 && geteuid() == getuid());
    assert(getegid() != 0 && getegid() == getgid());

    hikari_server_start(options.config_path, options.autostart);
    hikari_server_stop();

    return EXIT_SUCCESS;
  }
}
