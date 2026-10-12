# Switch artwork

`LeftSwitchTrack.png` and `RightSwitchTrack.png` are orthographic top-view renders
of LDraw assemblies `53407c01.dat` (left) and `53404c01.dat` (right).
The models include the switch blade, underside cover, actuator, and yellow lever.

Source: https://library.ldraw.org/library/unofficial/parts/53407c01.zip
Source: https://library.ldraw.org/library/unofficial/parts/53404c01.zip
Downloaded October 11, 2026. These assemblies are unofficial LDraw parts.

The assembly and switch body were authored by Jens Brühl [jb70].
Dependencies retain their author and license notices in `ldraw-switches/`;
contributors include Ronald Vallenduuk [Duq] and the other authors named there.
The models and derived graphics are licensed under Creative Commons Attribution
4.0: https://creativecommons.org/licenses/by/4.0/
Changes: top-view projection, shading, transparent raster output, and calibration.
The end-connector submesh is reflected across each rail centerline to match the
physical tongue/socket orientation and the application's straight/curved track.
End-sleeper seam positions and rail connection directions are preserved.

Regenerate with `python3 tools/render_switches.py` (requires Pillow and NumPy).
Each image covers a 24 x 36 stud canvas at 40 pixels per stud. The entrance
center is one stud below the top; the straight route spans 32 studs. The main
line is four studs from the left edge on the right switch, four from the right
edge on the left switch. The diverging connector offsets are measured from the
end-sleeper transforms in the source models, not from the image bounds.
