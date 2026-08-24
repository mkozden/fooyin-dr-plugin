# fooyin Dynamic Range Meter

An out-of-tree fooyin plugin that measures the classic TT-style dynamic range
score for selected tracks. It supports per-track scans, treating a selection
as one album, and splitting a selection into albums by album artist, date, and
album tags, with an explicit review step before tags are written.

The meter writes plain integer values to the compatible `DYNAMIC RANGE` and
`ALBUM DYNAMIC RANGE` metadata fields. It is an independent, compatibility-
oriented implementation and does not claim certification as an official DR
meter.

## Build

Requirements: fooyin development package 0.12.6 or newer, Qt 6, CMake 3.24,
and a C++23 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build
```

Set `DRMETER_PLUGIN_INSTALL_DIR` at configure time to override the plugin
destination reported by fooyin's CMake package.

After installation, restart fooyin. The scan commands appear under the selected
tracks' **Dynamic Range Meter** context submenu.

## Limitations

- The MVP does not calculate true peak, LUFS, or EBU R128 loudness range.
- Remote, archived, cue, and chapter tracks can be measured if fooyin can decode
  them, but their results are display-only.
- fooyin currently publishes plugin ABI version `0.0`; rebuild this plugin after
  fooyin upgrades that may change its C++ ABI.
