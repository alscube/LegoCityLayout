# Flex track artwork and geometry

The flex track is LEGO part 64022 (BrickLink assembly 88492c00), made from
female half 88492 and male half 88493. Official LDraw models by Philippe Hurbain
[Philo], downloaded October 11, 2026:

- https://library.ldraw.org/library/official/parts/64022.dat
- https://library.ldraw.org/library/official/parts/88492.dat
- https://library.ldraw.org/library/official/parts/88493.dat

Models and derived graphics are licensed CC BY 4.0:
https://creativecommons.org/licenses/by/4.0/
Dependencies retain their author and license notices in `ldraw-flex-track/`.
Changes: orthographic rendering, stud/rail highlights, shadows, and gray matched
to straight track. The projection preserves the tongue/socket arrangement used
by the application's straight and curved track.

Run `python3 tools/render_flex_track.py` with NumPy and Pillow to regenerate.
The images use 40 pixels/stud on an 8 x 10 stud transparent canvas. The physical
segment is 4 studs long and 8 studs wide; padding leaves room for its connectors
and bend. End centers are (2, 5) and (6, 5), with the hinge at (4, 5).
The two model halves rotate about their shared origin. The official 64022 model
specifies the maximum bend as +/- 5.625 degrees. The entry half stays fixed;
the exit half rotates. Graphics, connection positions, and directions use the
same angle, which is serialized independently from whole-element rotation.
