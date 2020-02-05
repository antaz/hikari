#include <hikari/layout.h>

#include <assert.h>

#include <hikari/split.h>
#include <hikari/tile.h>
#include <hikari/view.h>

void
hikari_layout_init(struct hikari_layout *layout, struct hikari_split *split)
{
  layout->split = split;
  wl_list_init(&layout->tiles);
}

void
hikari_layout_fini(struct hikari_layout *layout)
{
  /* hikari_split_fini(layout->split); */
}

#define CYCLE_LAYOUT(name, link)                                               \
  struct hikari_view *hikari_layout_##name##_view(                             \
      struct hikari_layout *layout)                                            \
  {                                                                            \
    struct wl_list *link = layout->tiles.link;                                 \
    struct hikari_tile *link##_tile;                                           \
                                                                               \
    do {                                                                       \
      if (link == &layout->tiles) {                                            \
        return NULL;                                                           \
      } else {                                                                 \
        link##_tile = wl_container_of(link, link##_tile, layout_tiles);        \
      }                                                                        \
      link = link->link;                                                       \
    } while (hikari_view_is_hidden(link##_tile->view));                        \
                                                                               \
    assert(link##_tile != NULL);                                               \
    assert(link##_tile->view != NULL);                                         \
    assert(!hikari_view_is_hidden(link##_tile->view));                         \
                                                                               \
    return link##_tile->view;                                                  \
  }

CYCLE_LAYOUT(first, next)
CYCLE_LAYOUT(last, prev)
#undef CYCLE_LAYOUT
