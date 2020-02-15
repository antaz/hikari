# Hikari - Wayland Compositor

![Screenshot](https://acmelabs.space/~raichoo/hikari.png)

## Description

_hikari_ is a stacking Wayland compositor with additional tiling capabilities,
it is heavily inspired by the Calm Window manager (cwm(1)). Its core concepts
are *views*, *groups*, *sheets* and the *workspace*.

The workspace is the set of views that are currently visible.

A sheet is a collection of views, each view can only be a member of a single
sheet. Switching between sheets will replace the current content of the
workspace with all the views that are a member of the selected sheet. _hikari_
has 9 general purpose sheets that correspond to the numbers **1** to **9** and a
special purpose sheet **0**. Views that are a member of sheet **0** will
always be visible but stacked below the views of the selected sheet.

Groups are a bit more fine grained than sheets. Like sheets, groups are a
collection of views. Unlike sheets you can have a arbitrary number of groups
and each group can have an arbitrary name. Views from one group can be spread
among all available sheets. Some operations act on entire groups rather than
individual views.

Please note that `hikari` is currently in `alpha` state and Wayland still
requires some work on FreeBSD. This release is targeted towards people who want
to help improving `hikari` by either providing feedback, patches
and/or help improving Wayland on FreeBSD.

## Setting up Wayland on FreeBSD

Wayland currently requires some care to work properly on FreeBSD. This section
aims to document the recent state of how to enable Wayland on the FreeBSD
`STABLE` branch and will change once support is being improved.

### Local `devfs.rules`

NOTE: this should only be used on single user setups since it allows members of
the `video` group to gain access to the `input` devices. This is a temporary
solution until FreeBSD offers ways to handle this in a better way.

Add the following to `/etc/devfs.rules`

```
[localrules=10]
add path 'input/*' mode 0660 group video
```

Add `devfs_system_ruleset="localrules"` to `/etc/rc.conf`.

### Start `moused`

Some systems might require `moused` for mice to work. Enable it with `service
moused enable`

### Setting up XDG\_RUNTIME\_DIR

This section describes how to use `/tmp` as your `XDG_RUNTIME_DIR`. Some Wayland
clients (e.g. native Wayland `firefox`) require `posix_fallocate` to work in
that directory. This is not supported by ZFS, therefore you should prevent the
ZFS tmp dataset from mounting to `/tmp` and `mount -t tmpfs tmpfs /tmp`. To
persist this setting edit your `/etc/fstab` appropriately to automatically mount
`tmpfs` during boot.

Additionally set `XDG_RUNTIME_DIR` to `/tmp` in your environment.

### Setting up XKB

`hikari` currently gets its `xkb` settings settings the appropriate environment
variables to something like the following.

```
XKB_DEFAULT_RULES "evdev"
XKB_DEFAULT_LAYOUT "de(nodeadkeys),de"
```

## Building

`hikari` currently only works on FreeBSD and Linux. This will likely change in
the future when more systems adopt Wayland.

### Dependencies

* wlroots
* pango
* cairo
* libinput
* xkbcommon
* pixman
* libucl
* glib
* epoll-shim

### Compiling and Installing

The build process will produce two binaries `hikari` and `hikari-unlocker`. The
latter one is used to check credentials for unlocking the screen. Both need to
be installed with root setuid in your `PATH`.

`hikari` can be configured via `$HOME/.config/hikari/hikari.conf`, an example
can be found under `doc/example_hikari.conf`.

#### Building on FreeBSD

Simply run `make`.

#### Building on Linux

On Linux `bmake` is required which needs to be run like so:

```
bmake WITH_POSIX_C_SOURCE=YES
```

#### Building with XWayland support

`hikari` offers optional XWayland support which is enabled via setting
`WITH_XWAYLAND`.

Example:

```
make WITH_XWAYLAND=YES
```

## TODO before 1.0

* Popups are not positioned correctly on multi monitor setups.
* Manpage is still missing.

## Community

The `hikari` community gears to be inclusive and welcoming to everyone, this is
why we chose to adere to the [Geekfeminism Code of
Conduct](https://geekfeminismdotorg.wordpress.com/about/code-of-conduct/).

If you care to be a part of our community, please join our Matrix chat at
`#hikari:acmelabs.space` and/or subscribe to our mailing list by sending a mail
to `hikari+subscribe@acmelabs.space`.

## Contributing

Please make sure you use `clang-format` with the accompanying `.clang-format`
configuration before submitting any patches.
