# Changelog

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
