# Changelog

## 2.3.3

* update to `wloots` 0.15.0

## 2.3.2

* fix crash on startup
* fix flickering popups in `firefox`

## 2.3.1

* update to `wloots` 0.14.0

## 2.3.0

* improvements for multi-monitor group operations
* allow environment variables to be used in the configuration
* add `accel-profile` configuration
* allow `pkg-config` to be specified to support more build environments
* do not rely on suid by default anymore (seatd is now preferred)
* update to `wlroots` 0.13.0

## 2.2.3

* fix noop resizes
* fix subsurface handling (causes `firefox` 87 issues)

## 2.2.2

* fix `input-grab-mode` for non-default bindings

## 2.2.1

* NULL pointer check on keyboard_config merge
* Fixed typo in the manpage
* Recompute focus on view pinning

## 2.2.0

* add support for virtual input (`wayvnc` support)
* add middle click emulation
* add fallback layouts
* add child view configuration for native Wayland views
* add output relative view positioning actions
* add `WITH_ALL` compile option
* improved handling of maximized views
* add graceful shutdown
* multi monitor improvements for Xwayland and locking
* Fix assert fail with layout select and invisible views
* Indicate focus view on layout select
* clear focus on mark select
* Fix indicator bar coloring bug

## 2.1.2

* Typo fixes in manpage
* Fix double free on configuration reload

## 2.1.1

* Allow geometry changes to unmapped surface (fixes `mako`)
* handle `NULL` app id
* check if surface is valid on input grab cursor move (fixes `wlroots` assert)

## 2.1.0

* update to `wlroots` 0.11.0
* new lockscreen
* add public view flag to include views on the lockscreen
* add noop output to handle deallocation of all outputs
* inhabited sheet cycling is now wrapping
* add configuration for switches (e.g. lid switch)
* add keyboard configuration
* keyboard repeat rate/delay configuration
* add view preview on cycling
* improved renderer performance
* fix mouse dragging bug

## 2.0.5

* fix unmap/map handling for layers
* resolve workspace on NULL output

## 2.0.4

* fix focus issue on cycling
* factor in bottom layer for usable area calculations
* fix minor typo in manpage

## 2.0.3

* disallow running `hikari` as root
* unset cycling bit when lowering a view

## 2.0.2

* fix nested popups in layer-shell
* fix tiling state handling for unmapped views
* fix exclusive area handling for output relative positions

## 2.0.1

* fix timing issue with XWayland view unmap/map
* remove `hikari.desktop` on `uninstall`
* clean up Makefile `PREFIX`

## 2.0.0

* floating views are raised after layout apply
* sheet is reset before layout apply
* focus view is raised to layout on layout apply
* add append/prepend to layout operations
* remove sheet groups (ungrouped views create a group for their app id instead)
* groups can now start with digits (no more sheet group overlap)
* `PREFIX` is needed for all make step (compile in configuration paths)
* add `sheet-show-group` operation
* add `workspace-show-group` operation
* add `workspace-show-invisible` operation
* add `workspace-show-all` operation
* add `workspace-clear` operation
* add `workspace-show-group` operation
* add `workspace-cycle-[next|prev]` operations
* add output relative view position configuration (e.g. center, bottom-right)
* allow tiled views to be moved around
* migrate views to other outputs using move operations (mouse and keyboard)
* add move libinput configuration options for pointer devices
* add `ui` section to configuration
* add default configuration file
* add default wallpaper
* add `hikari.desktop` session file
* many multi-monitor fixes
* and many more bugfixes

## 1.2.1

* fix double selection manager creation
* add CHANGELOG.md to distribution tarball

## 1.2.0

* add drag/drop support
* add client side cursor setting
* add invisible view configuration flag
* add floating view configuration flag
* fix workspace focus on multimonitor

## 1.1.1

* Xwayland fix for multimonitor coordinates
* fix tile exchange with maximized views
* fix layer-shell destroy issue

## 1.1.0

* add layer-shell support
* add begin/end action to bindings
* add "remove word" to input handling
* allow Ctrl-C and Ctrl-D to cancel modes
* add `step` configuration
* support xdg toplevel decoration events (fixes `alacritty` issues)
* add symmetric resize operations
* add dynamic layout scaling

## 1.0.4

* fix coordinated for XWayland views on multi-monitor setups

## 1.0.3

* fix Xwayland unmap/map
* fix crash on cycling borrowed tiled views on tiled sheet
* fix damage for subsurface on pending operations
* fix `set_title` un/map for xdg views
* fix center cursor on exchange

## 1.0.2

* fix "frame done" for hidden xwayland views leading to crashes

## 1.0.1

* update cursor focus after tiling
* fix tile detach on view reset
* send "frame done" to all output views
* manpage typo fixes and additions
