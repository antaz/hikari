#if !defined(HIKARI_OPTION_H)
#define HIKARI_OPTION_H

#include <assert.h>
#include <stdbool.h>

#define HIKARI_OPTION(name, type)                                              \
  struct {                                                                     \
    bool configured;                                                           \
    type value;                                                                \
  } name

#define HIKARI_OPTION_FUNS(name, option, type)                                 \
                                                                               \
  static inline void hikari_##name##_config_init_##option(                     \
      struct hikari_##name##_config *name##_config, type value)                \
  {                                                                            \
    name##_config->option.configured = false;                                  \
    name##_config->option.value = value;                                       \
  }                                                                            \
                                                                               \
  static inline void hikari_##name##_config_set_##option(                      \
      struct hikari_##name##_config *name##_config, type value)                \
  {                                                                            \
    name##_config->option.configured = true;                                   \
    name##_config->option.value = value;                                       \
  }                                                                            \
                                                                               \
  static inline type hikari_##name##_config_get_##option(                      \
      struct hikari_##name##_config *name##_config)                            \
  {                                                                            \
    assert(name##_config->option.configured);                                  \
    return name##_config->option.value;                                        \
  }                                                                            \
                                                                               \
  static inline bool hikari_##name##_config_has_##option(                      \
      struct hikari_##name##_config *name##_config)                            \
  {                                                                            \
    return name##_config->option.configured;                                   \
  }                                                                            \
                                                                               \
  static inline bool hikari_##name##_config_merge_##option(                    \
      struct hikari_##name##_config *name##_config,                            \
      struct hikari_##name##_config *name##_default_config)                    \
  {                                                                            \
    if (!hikari_##name##_config_has_##option(name##_config)) {                 \
      hikari_##name##_config_set_##option(                                     \
          name##_config, name##_default_config->option.value);                 \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }

#endif
