OS != uname
VERSION ?= "CURRENT"
PREFIX ?= /usr/local
ETC_PREFIX ?= ${PREFIX}

OBJS = \
	action.o \
	action_config.o \
	bindings.o \
	border.o \
	command.o \
	completion.o \
	configuration.o \
	cursor.o \
	decoration.o \
	dnd_mode.o \
	exec.o \
	font.o \
	geometry.o \
	group.o \
	group_assign_mode.o \
	indicator.o \
	indicator_bar.o \
	indicator_frame.o \
	input_buffer.o \
	input_grab_mode.o \
	keyboard.o \
	layer_shell.o \
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
	position_config.o \
	resize_mode.o \
	server.o \
	sheet.o \
	sheet_assign_mode.o \
	split.o \
	tile.o \
	unlocker.o \
	view.o \
	view_autoconf.o \
	workspace.o \
	xdg_view.o \
	xwayland_unmanaged_view.o \
	xwayland_view.o

WAYLAND_PROTOCOLS != pkg-config --variable pkgdatadir wayland-protocols

.PHONY: distclean clean clean-doc doc dist install uninstall
.PATH: src

# Allow specification of /extra/ CFLAGS and LDFLAGS
CFLAGS += ${CFLAGS_EXTRA}
LDFLAGS += ${LDFLAGS_EXTRA}

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

.ifdef WITH_LAYERSHELL
CFLAGS += -DHAVE_LAYERSHELL=1
.endif

CFLAGS += -Wall -I. -Iinclude -DHIKARI_ETC_PREFIX=${ETC_PREFIX}

WLROOTS_CFLAGS != pkg-config --cflags wlroots
WLROOTS_LIBS != pkg-config --libs wlroots

WLROOTS_CFLAGS += -DWLR_USE_UNSTABLE=1

PANGO_CFLAGS != pkg-config --cflags pangocairo
PANGO_LIBS != pkg-config --libs pangocairo

CAIRO_CFLAGS != pkg-config --cflags cairo
CAIRO_LIBS != pkg-config --libs cairo

GLIB_CFLAGS != pkg-config --cflags glib-2.0
GLIB_LIBS != pkg-config --libs glib-2.0

PIXMAN_CFLAGS != pkg-config --cflags pixman-1
PIXMAN_LIBS != pkg-config --libs pixman-1

XKBCOMMON_CFLAGS != pkg-config --cflags xkbcommon
XKBCOMMON_LIBS != pkg-config --libs xkbcommon

WAYLAND_CFLAGS != pkg-config --cflags wayland-server
WAYLAND_LIBS != pkg-config --libs wayland-server

LIBINPUT_CFLAGS != pkg-config --cflags libinput
LIBINPUT_LIBS != pkg-config --libs libinput

UCL_CFLAGS != pkg-config --cflags libucl
UCL_LIBS != pkg-config --libs libucl

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

version.h:
	echo "#define HIKARI_VERSION \"${VERSION}\"" >> version.h

hikari: version.h xdg-shell-protocol.h wlr-layer-shell-unstable-v1-protocol.h ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} ${INCLUDES} -o ${.TARGET} ${OBJS} ${LIBS}

xdg-shell-protocol.h:
	wayland-scanner server-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${.TARGET}

wlr-layer-shell-unstable-v1-protocol.h:
	wayland-scanner server-header protocol/wlr-layer-shell-unstable-v1.xml ${.TARGET}

hikari-unlocker: hikari_unlocker.c
	${CC} ${CFLAGS_EXTRA} ${LDFLAGS_EXTRA} -o hikari-unlocker hikari_unlocker.c -lpam

clean-doc:
	@test -e _darcs && echo "cleaning manpage" ||:
	@test -e _darcs && rm share/man/man1/hikari.1 2> /dev/null ||:

clean: clean-doc
	@echo "cleaning headers"
	@test -e _darcs && rm version.h ||:
	@rm xdg-shell-protocol.h 2> /dev/null ||:
	@rm wlr-layer-shell-unstable-v1-protocol.h 2> /dev/null ||:
	@echo "cleaning object files"
	@rm ${OBJS} 2> /dev/null ||:
	@echo "cleaning executables"
	@rm hikari 2> /dev/null ||:
	@rm hikari-unlocker 2> /dev/null ||:

share/man/man1/hikari.1:
	pandoc -M title:"HIKARI(1) ${VERSION} | hikari - Wayland Compositor" -s \
		--to man -o share/man/man1/hikari.1 share/man/man1/hikari.md

doc: share/man/man1/hikari.1

hikari-${VERSION}.tar.gz: version.h share/man/man1/hikari.1
	@darcs revert
	@tar -s "#^#hikari-${VERSION}/#" -czf hikari-${VERSION}.tar.gz \
		version.h \
		main.c \
		hikari_unlocker.c \
		include/hikari/*.h \
		src/*.c \
		protocol/*.xml \
		Makefile \
		LICENSE \
		README.md \
		share/man/man1/hikari.md \
		share/man/man1/hikari.1 \
		etc/hikari/hikari.conf \
		etc/pam.d/hikari-unlocker.*

distclean: clean-doc
	@test -e _darcs && echo "cleaning version.h" ||:
	@test -e _darcs && rm version.h ||:

dist: distclean hikari-${VERSION}.tar.gz

install: hikari hikari-unlocker share/man/man1/hikari.1
	mkdir -p ${DESTDIR}/${PREFIX}/bin
	mkdir -p ${DESTDIR}/${PREFIX}/share/man/man1
	mkdir -p ${DESTDIR}/${ETC_PREFIX}/etc/hikari
	mkdir -p ${DESTDIR}/${ETC_PREFIX}/etc/pam.d
	install -m 4555 hikari hikari-unlocker ${DESTDIR}/${PREFIX}/bin
	install -m 644 share/man/man1/hikari.1 ${DESTDIR}/${PREFIX}/share/man/man1
	install -m 644 etc/hikari/hikari.conf ${DESTDIR}/${ETC_PREFIX}/etc/hikari
	install -m 644 etc/pam.d/hikari-unlocker.${OS} ${DESTDIR}/${ETC_PREFIX}/etc/pam.d/hikari-unlocker

uninstall:
	-rm ${DESTDIR}/${PREFIX}/bin/hikari
	-rm ${DESTDIR}/${PREFIX}/bin/hikari-unlocker
	-rm ${DESTDIR}/${PREFIX}/share/man/man1/hikari.1
	-rm ${DESTDIR}/${ETC_PREFIX}/etc/pam.d/hikari-unlocker
	-rm ${DESTDIR}/${ETC_PREFIX}/etc/hikari/hikari.conf
	-rmdir ${DESTDIR}/${ETC_PREFIX}/etc/hikari
