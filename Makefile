.ifmake doc || dist
.ifndef VERSION
.error please specify VERSION
.endif
.endif

OBJS = \
	action_config.o \
	border.o \
	command.o \
	completion.o \
	configuration.o \
	exec.o \
	font.o \
	geometry.o \
	group.o \
	group_assign_mode.o \
	indicator.o \
	indicator_bar.o \
	indicator_frame.o \
	input_buffer.o \
	keyboard.o \
	keyboard_grab_mode.o \
	layout.o \
	layout_config.o \
	layout_select_mode.o \
	main.o \
	mark.o \
	mark_assign_mode.o \
	mark_select_mode.o \
	maximized_state.o \
	memory.o \
	move_mode.o \
	normal_mode.o \
	output.o \
	output_config.o \
	pointer.o \
	pointer_config.o \
	resize_mode.o \
	server.o \
	sheet.o \
	sheet_assign_mode.o \
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

WAYLAND_PROTOCOLS ?= `pkg-config --variable pkgdatadir wayland-protocols`

.PHONY: clean doc dist
.PATH: src

.ifdef DEBUG
CFLAGS += -g -O0 -fsanitize=address
.else
CFLAGS += -DNDEBUG
.endif

.ifdef WITH_POSIX_C_SOURCE
CFLAGS += -D_POSIX_C_SOURCE=200809L
.endif

.ifdef WITH_XWAYLAND
CFLAGS += -DHAVE_XWAYLAND=1
.endif

.ifdef WITH_GAMMACONTROL
CFLAGS += -DHAVE_GAMMACONTROL=1
.endif

.ifdef WITH_SCREENCOPY
CFLAGS += -DHAVE_SCREENCOPY=1
.endif

CFLAGS += -Wall -I. -Iinclude

WLROOTS_CFLAGS ?= `pkg-config --cflags wlroots`
WLROOTS_LIBS ?= `pkg-config --libs wlroots`

WLROOTS_CFLAGS += -DWLR_USE_UNSTABLE=1

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
	@test -e _darcs && echo "cleaning manpage" ||:
	@test -e _darcs && rm share/man/man1/hikari.1 2> /dev/null ||:
	@echo "cleaning headers"
	@rm xdg-shell-protocol.h 2> /dev/null ||:
	@echo "cleaning object files"
	@rm ${OBJS} 2> /dev/null ||:
	@echo "cleaning executables"
	@rm hikari 2> /dev/null ||:
	@rm hikari-unlocker 2> /dev/null ||:

debug: hikari hikari-unlocker

share/man/man1/hikari.1!
	@sed '1s/VERSION/${VERSION}/' share/man/man1/hikari.md |\
		pandoc --standalone --to man -o share/man/man1/hikari.1

doc: share/man/man1/hikari.1

hikari-${VERSION}.tar.gz: share/man/man1/hikari.1
	@darcs revert
	@tar -s "#^#hikari-${VERSION}/#" -czf hikari-${VERSION}.tar.gz \
		main.c \
		hikari_unlocker.c \
		include/hikari/*.h \
		src/*.c \
		Makefile \
		LICENSE \
		README.md \
		share/man/man1/hikari.md \
		share/man/man1/hikari.1 \
		share/examples/hikari.conf \
		pam.d/hikari-unlocker.freebsd \
		pam.d/hikari-unlocker.linux

dist: hikari-${VERSION}.tar.gz
