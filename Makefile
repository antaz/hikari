OBJS = \
	background.o \
	border.o \
	command.o \
	completion.o \
	configuration.o \
	exec.o \
	exec_select_mode.o \
	font.o \
	geometry.o \
	group.o \
	group_assign_mode.o \
	indicator.o \
	indicator_bar.o \
	indicator_frame.o \
	input_buffer.o \
	keybinding.o \
	keyboard.o \
	keyboard_grab_mode.o \
	layout.o \
	main.o \
	mark.o \
	mark_assign_mode.o \
	mark_select_mode.o \
	maximized_state.o \
	memory.o \
	move_mode.o \
	normal_mode.o \
	output.o \
	pointer_config.o \
	resize_mode.o \
	server.o \
	sheet.o \
	split.o \
	tile.o \
	tiling_mode.o \
	unlocker.o \
	view.o \
	view_autoconf.o \
	workspace.o \
	xdg_view.o \
	xwayland_unmanaged_view.o \
	xwayland_view.o

WAYLAND_PROTOCOLS = /usr/local/share/wayland-protocols

.PHONY: clean debug
.PATH: src

.ifmake debug
CFLAGS += -g -O0 -fsanitize=address
.else
CFLAGS += -DNDEBUG
.endif

CFLAGS += -Wall -I. -Iinclude

WLROOTS_CFLAGS ?= `pkg-config --cflags wlroots`
WLROOTS_LIBS ?= `pkg-config --libs wlroots`

WLROOTS_CFLAGS += -DWLR_USE_UNSTABLE=1

EPOLLSHIM_LIBS ?= `pkg-config --libs epoll-shim`

PANGO_CFLAGS ?= `pkg-config --cflags pangocairo`
PANGO_LIBS ?= `pkg-config --libs pangocairo`

CAIRO_CFLAGS ?= `pkg-config --cflags cairo`
CAIRO_LIBS ?= `pkg-config --libs cairo`

GLIB_CFLAGS ?= `pkg-config --cflags glib-2.0`
GLIB_LIBS ?= `pkg-config --libs glib-2.0`

PIXMAN_CFLAGS ?= `pkg-config --cflags pixman-1`
PIXMAN_LIBS ?= `pkg-config --libs pixman-1`

XKBCOMMON_CFLAGS ?= `pkg-config --cflags xkbcommon`
XKBCOMMON_LIBS ?= `pkg-config --libs xkbcommon`

WAYLAND_CFLAGS ?= `pkg-config --cflags wayland-server`
WAYLAND_LIBS ?= `pkg-config --libs wayland-server`

LIBINPUT_CFLAGS ?= `pkg-config --cflags libinput`
LIBINPUT_LIBS ?= `pkg-config --libs libinput`

UCL_CFLAGS ?= `pkg-config --cflags libucl`
UCL_LIBS ?= `pkg-config --libs libucl`

CFLAGS += \
	${WLROOTS_CFLAGS} \
	${PANGO_CFLAGS} \
	${CAIRO_CFLAGS} \
	${GLIB_CFLAGS} \
	${PIXMAN_CFLAGS} \
	${XKBCOMMON_CFLAGS} \
	${WAYLAND_CFLAGS} \
	${LIBINPUT_CFLAGS} \
	${UCL_CFLAGS}

LIBS = \
	${WLROOTS_LIBS} \
	${EPOLLSHIM_LIBS} \
	${PANGO_LIBS} \
	${CAIRO_LIBS} \
	${GLIB_LIBS} \
	${PIXMAN_LIBS} \
	${XKBCOMMON_LIBS} \
	${WAYLAND_LIBS} \
	${LIBINPUT_LIBS} \
	${UCL_LIBS}

all: hikari hikari-unlocker

hikari: xdg-shell-protocol.h ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} ${INCLUDES} ${LIBS} ${OBJS} -o ${.TARGET}

xdg-shell-protocol.h:
	wayland-scanner server-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${.TARGET}

hikari-unlocker: hikari_unlocker.c
	${CC} -lpam hikari_unlocker.c -o hikari-unlocker

clean:
	rm *.o ||:
	rm hikari ||:
	rm hikari-unlocker ||:
	rm xdg-shell-protocol.h ||:

debug: hikari hikari-unlocker
