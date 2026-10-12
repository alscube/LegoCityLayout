"""Render the official flex-track assembly and its independently hinged halves."""
from pathlib import Path
import render_switches as renderer
root=Path(__file__).resolve().parents[1]
renderer.LIB=root/'resources'/'ldraw-flex-track'
for model, name in [('64022.dat','FlexTrack.png'),('88492.dat','FlexTrackFemale.png'),('88493.dat','FlexTrackMale.png')]:
    renderer.render(model,root/'resources'/name,canvas=(8,10),origin=(4,5),horizontal=True)
