"""Build Bambu Studio projects (.3mf) from the case STLs.

Maintainer tool: needs the bambu3mf / printcheck helpers (Bambu Lab A1, Bambu PLA Basic profile)
from a separate repository; point BAMBU_DIR at that folder. Regenerate the STLs with OpenSCAD
first (see ziggo-harmony-case.scad).

    BAMBU_DIR=C:/proj/stl-models/bambu python make_3mf.py
"""
import os
import sys
from pathlib import Path

import manifold3d as m3d
import numpy as np
import trimesh

HERE = Path(__file__).parent
sys.path.insert(0, os.environ.get("BAMBU_DIR", "C:/proj/stl-models/bambu"))
from bambu3mf import BRIM, FIJN, MASSIEF, STANDAARD, schrijf_project  # noqa: E402
from printcheck import uitsteek, zwevend  # noqa: E402


def load(name):
    return trimesh.load(HERE / f"ziggo-harmony-{name}.stl", force="mesh")


def check(name, tm):
    m = m3d.Manifold(m3d.Mesh(vert_properties=np.asarray(tm.vertices, dtype=np.float32),
                              tri_verts=np.asarray(tm.faces, dtype=np.uint32)))
    print(f"{name}: {tm.extents.round(1)} mm, floating={zwevend(m)}, overhangs={uitsteek(m)}")


base, lid, pipe = load("base"), load("lid"), load("pipe")
for n, tm in (("base", base), ("lid", lid), ("pipe", pipe)):
    check(n, tm)

# Case in one colour; walls a bit stronger for the snap fit.
schrijf_project(HERE / "ziggo-harmony-case.3mf",
                [("onderbak", base, {}), ("deksel", lid, {})],
                globaal={**STANDAARD, "wall_loops": "3"})
# Light pipes in transparent filament: solid, fine layers, a brim against tipping over.
schrijf_project(HERE / "ziggo-harmony-light-pipe.3mf",
                [(f"lichtgeleider {i}", pipe, {}) for i in range(1, 5)],
                globaal={**FIJN, **MASSIEF, **BRIM}, tussenruimte=8.0)
print("written:", *sorted(p.name for p in HERE.glob("*.3mf")))
