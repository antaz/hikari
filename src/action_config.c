#include <hikari/action_config.h>

#include <string.h>

#include <hikari/memory.h>
#include <hikari/split.h>

void
hikari_action_config_init(struct hikari_action_config *action_config,
    const char *action_name,
    const char *command)
{
  size_t action_name_len = strlen(action_name);
  action_config->action_name = hikari_malloc(action_name_len + 1);
  strcpy(action_config->action_name, action_name);

  size_t command_len = strlen(command);
  action_config->command = hikari_malloc(command_len + 1);
  strcpy(action_config->command, command);
}

void
hikari_action_config_fini(struct hikari_action_config *action_config)
{
  hikari_free(action_config->action_name);
  hikari_free(action_config->command);
}
