"""Persist the shader permutations used by foliage instances and Nanite proxy meshes."""

import os
import unreal as u

base = "/Game/NightOfTheWolves/"
forest = [
    "M_DampTread",
    "M_NeedleLitter",
    "M_FernFronds",
    "M_WeatheredWood",
    "M_fir_tree_01_Bark",
    "M_fir_tree_01_Needles",
    "M_pine_tree_01_Bark",
    "M_pine_tree_01_Needles",
]
shared = ["M_ForestGround", "M_CabinStone", "M_MossyPath", "M_TreeBark"]
for name in ["Cascades/" + n for n in forest] + ["Presentation/" + n for n in shared]:
    mat = u.EditorAssetLibrary.load_asset(base + name)
    mat.set_editor_properties({"used_with_instanced_static_meshes": True, "used_with_nanite": True})
    if os.environ.get("NW_VALIDATE_SHADERS"):
        errors = u.MaterialEditingLibrary.recompile_material(mat)
        if errors:
            raise RuntimeError(name + ": " + str(errors))
    u.EditorAssetLibrary.save_loaded_asset(mat, False)
    u.log("PREPARED_FOLIAGE_SHADER " + name)
