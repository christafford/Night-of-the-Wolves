"""Rebuild the cottage resident in UE 5.9 with MetaHuman Creator and Geometry Scripting.

Run the full editor on /Engine/Maps/Entry with -ExecutePythonScript pointing here.
The optional NOTW_HERMIT_PART selects face, pose, accessories, or outfit for an art iteration.
The actor references only saved runtime assets.
"""

import os
import sys
from pathlib import Path

import unreal as u

project = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(project / "Scripts"))
from hermit.appearance import build_accessories, build_face
from hermit.pose import build_aim
from hermit.workwear import build_outfit

part = os.environ.get("NOTW_HERMIT_PART", "all")
if part in ("all", "face"):
    build_face()
if part in ("all", "pose"):
    build_aim()
if part in ("all", "accessories"):
    build_accessories()
if part in ("all", "outfit"):
    build_outfit()
u.log("COTTAGE HERMIT READY " + part)
