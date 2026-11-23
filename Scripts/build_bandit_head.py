"""Build the fitted roadside character head and hat with the full Unreal Editor.

Use a renderer-backed editor for the first skin bake. NOTW_BANDIT_HEAD selects
'sculpt', 'skin', 'accessories' or 'pose' when iterating on one part.
"""

import os
import sys
from pathlib import Path

import unreal as u

sys.path.insert(
    0,
    str(Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())) / "Scripts"),
)
part = os.environ.get("NOTW_BANDIT_HEAD", "all")
if part in ("all", "sculpt", "skin"):
    from bandit.head import build

    build(part != "sculpt")
if part in ("all", "accessories"):
    from bandit.headwear import build

    build()
if part in ("all", "sculpt", "skin", "pose"):
    from bandit.demand import build

    build()
u.log("BANDIT HEAD READY " + part)
