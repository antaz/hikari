#include <hikari/completion.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <hikari/memory.h>

void
hikari_completion_init(struct hikari_completion *completion, char *data)
{
  struct hikari_completion_item *item;
  wl_list_init(&completion->items);
  hikari_completion_add(completion, data);
  item = wl_container_of(completion->items.next, item, completion_items);
  completion->current_item = item;
}

void
hikari_completion_fini(struct hikari_completion *completion)
{
  assert(completion != NULL);

  struct hikari_completion_item *item = NULL, *item_temp;

  wl_list_for_each_safe (
      item, item_temp, &completion->items, completion_items) {
    wl_list_remove(&item->completion_items);
    hikari_free(item);
  }
}

void
hikari_completion_add(struct hikari_completion *completion, char *data)
{
  assert(completion != NULL);

  struct hikari_completion_item *completion_item =
      hikari_malloc(sizeof(struct hikari_completion_item));

  strncpy(completion_item->data, data, sizeof(completion_item->data) - 1);

  wl_list_insert(completion->items.prev, &completion_item->completion_items);
}

char *
hikari_completion_cancel(struct hikari_completion *completion)
{
  assert(completion != NULL);
  assert(!wl_list_empty(&completion->items));

  struct hikari_completion_item *item =
      wl_container_of(completion->items.next, item, completion_items);

  completion->current_item = item;

  return item->data;
}

#define COMPLETION(link)                                                       \
  char *hikari_completion_##link(struct hikari_completion *completion)         \
  {                                                                            \
    assert(completion != NULL);                                                \
    assert(!wl_list_empty(&completion->items));                                \
                                                                               \
    struct wl_list *link;                                                      \
    struct hikari_completion_item *item;                                       \
                                                                               \
    link = completion->current_item->completion_items.link;                    \
    if (link == &completion->items) {                                          \
      item = wl_container_of(completion->items.link, item, completion_items);  \
    } else {                                                                   \
      item = wl_container_of(link, item, completion_items);                    \
    }                                                                          \
                                                                               \
    completion->current_item = item;                                           \
                                                                               \
    return item->data;                                                         \
  }

COMPLETION(next)
COMPLETION(prev)
#undef COMPLETION
