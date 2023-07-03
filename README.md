# Kurai - Wayland Compositor forked from Hikari

![Screenshot](https://acmelabs.space/~raichoo/hikari.png)

### Dependencies

* wlroots
* pango
* cairo
* libinput
* xkbcommon
* pixman
* libucl
* evdev-proto
* epoll-shim (FreeBSD)
* XWayland (optional, runtime dependency)

### Compiling and Installing

#### Building on FreeBSD

Simply run `make`. The installation destination can be configured by setting
`PREFIX` (default is `/usr/local` and does not need to be given explicitly).

```
make
```

`uninstall` requires the same values for prefixes.

#### Building on Linux

On Linux `bmake` is required which needs to be run like so:

```
bmake WITH_POSIX_C_SOURCE=YES
```

The installation destination can be configured by
setting`PREFIX` (default is `/usr/local` and does not need to be given
explicitly).

```
bmake PREFIX=/usr/local install
```

`uninstall` requires the same values for prefixes.

### Building with all features enabled

The following sections explain how to enabled features on an individual basis.
However, to enable every feature the build system offers the `WITH_ALL` flag.

```
make WITH_ALL=YES
```

#### Building with XWayland support

```
make WITH_XWAYLAND=YES
```

#### Building with screencopy support

```
make WITH_SCREENCOPY=YES
```

#### Building with gammacontrol support

Gamma control is needed for tools like `redshift`. This is disabled by default
and can be enabled via setting `WITH_GAMMACONTROL`.

```
make WITH_GAMMACONTROL=YES
```

#### Building with layer-shell support

Some applications that are used to build desktop components require
`layer-shell`. Examples for this are `waybar`, `wofi` and `slurp`. To turn on
`layer-shell` support compile with the `WITH_LAYERSHELL` option.

```
make WITH_LAYERSHELL=YES
```

#### Building with virtual input support

Virtual input support is needed for applications like `wayvnc`.

```
make WITH_VIRTUAL_INPUT=YES
```
#### Installing with SUID

```
make WITH_SUID=YES install
```

#### Building a DEBUG build

```
make DEBUG=YES
```
## Contributing

Please make sure you use `clang-format` with the accompanying `.clang-format`
configuration before submitting any patches.
