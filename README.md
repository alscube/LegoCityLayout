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

Flex Track represents one 4-stud segment. Right-click it to adjust the bend
between -5.625 and +5.625 degrees, or straighten it. Whole-element rotation uses
5.625-degree steps. The bend and rotation are preserved in saved layouts.
See [flex artwork and geometry](resources/FLEX_TRACK_ARTWORK.md).

## Track run shortcuts

After dropping a track, use `+` (or `=`) on the layout canvas to attach another
of the same type to the end of the active run. Curves follow the rail direction;
flex tracks keep their bend. Use `-` to remove the last piece from that run.
Keypad plus and minus work too. Switches are excluded from repetition.

Connected drops continue the run; an unconnected track drop starts a new run.
Run order is independent of drawing order and kept separately for each open
project. Select an existing track and press `+` to start a run when no run is
active. The canvas must have keyboard focus; dropping a track focuses it.

## Saved layouts

Layout files contain table geometry, view coordinates, and each element's stable
type name and placement (position, rotation, and flex bend when applicable).
Artwork and stud dimensions come from the app's element catalog when opening a
layout; images are not embedded in the file.

## Plate definitions

The palette reads `Elements/Plates/*.info` and the PNG with the same basename.
Each UTF-8 `.info` file has four lines, for example:

```text
Plates
16,32
Green 16x32
0.75
```

These specify the tab, width and height in studs, label, and preview scale.
A scale of `0.0` uses the normal preview size; positive values multiply that
size. Empty labels/tabs, invalid dimensions/scales, and missing or unreadable
PNGs are skipped with a diagnostic. PNGs without an `.info` file are ignored.

Add matching files and restart the app to pick up a new plate; no new C++ class
or resource entry is needed. All plate types use their filename stem as the
stable ID in saved layouts.
Development builds read the source folder. Builds and installs also copy
`Elements` into the app's resources (macOS) or beside the executable (other
platforms) for use when the source folder is unavailable.
