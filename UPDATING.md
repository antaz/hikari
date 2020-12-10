## 2.0.0

### 20201210

* `suid` for `hikari` is no longer the default. We rely on `seatd` or similar
  mechanisms.

### 20200509

* f4c376ec920298361ccb3beb6031eef5c5dd7c39 adds the `ui` section to the
  configuration.

  `border`, `gap`, `step`, `font` and `colorscheme` need to be moved to the `ui`
  section. Refer to `etc/hikari/hikari.conf` for a working example.
