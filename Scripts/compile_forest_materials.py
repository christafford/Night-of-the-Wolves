"""Compile the two path/foliage materials in a renderer-enabled editor commandlet."""
import unreal

for name in ("M_MossyPath", "M_DarkFir"):
    mat = unreal.load_asset("/Game/NightOfTheWolves/Presentation/" + name)
    if not mat:
        raise RuntimeError("Missing material " + name)
    input_node = unreal.MaterialEditingLibrary.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    if not input_node:
        raise RuntimeError("Material has no base-color input: " + name)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("Compiled " + name + " with " + str(input_node.get_class().get_name()))
