#include <hikari/output_config.h>

#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include <hikari/memory.h>

void
hikari_output_config_init(
    struct hikari_output_config *output_config, const char *output_name)
{
  size_t output_name_len = strlen(output_name);
  output_config->output_name = hikari_malloc(output_name_len + 1);

  strcpy(output_config->output_name, output_name);

  struct hikari_position_config default_position;
  hikari_position_config_init(&default_position);

  hikari_output_config_init_background(output_config, NULL);
  hikari_output_config_init_background_fit(
      output_config, HIKARI_BACKGROUND_STRETCH);
  hikari_output_config_init_position(output_config, default_position);
}

void
hikari_output_config_fini(struct hikari_output_config *output_config)
{
  assert(output_config != NULL);

  hikari_free(output_config->output_name);
  hikari_free(output_config->background.value);
}

void
hikari_output_config_merge(struct hikari_output_config *output_config,
    struct hikari_output_config *default_config)
{
#define MERGE(option)                                                          \
  hikari_output_config_merge_##option(output_config, default_config);

  if (hikari_output_config_merge_background(output_config, default_config)) {
    char *background = default_config->background.value;

    if (background != NULL) {
      output_config->background.value = strdup(background);
    }
  }

  MERGE(background_fit);
  MERGE(position);
#undef MERGE
}
