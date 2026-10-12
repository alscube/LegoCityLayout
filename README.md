# LegoCityLayout

A Qt 6 Widgets desktop application for planning a LEGO city layout.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## License

Copyright 2026. Alan Krzywicki

This project is licensed under the GNU General Public License version 3
or (at your option) any later version (`GPL-3.0-or-later`). See [LICENSE](LICENSE)
for the full license text.

## Track checks

```sh
cmake -S . -B build -DLEGO_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The Tracks palette includes straight track, R40 curved track, and left/right
switches. All track elements rotate in 22.5-degree increments and snap at
compatible rail ends. Switches have three connection ends.

Switch artwork is derived from CC BY 4.0 LDraw models. See
[artwork attribution and regeneration](resources/SWITCH_ARTWORK.md).
