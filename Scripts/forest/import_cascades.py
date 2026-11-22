"""Import the lightweight canopy and fern models, then bind their photographic materials."""

from pathlib import Path
import os
import unreal

DEST = "/Game/NightOfTheWolves/Cascades"
SOURCE = Path(unreal.Paths.project_dir()) / "Art/Source/Cascades"
library = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def load_or_import(filename):
    name = Path(filename).stem
    path = DEST + "/" + name
    if filename.endswith(".glb"):
        path = path + "/StaticMeshes/" + name
    if library.does_asset_exist(path):
        asset = library.load_asset(path)
        if os.environ.get("NW_REIMPORT_FOREST"):
            manager = unreal.InterchangeManager.get_interchange_manager_scripted()
            params = unreal.ImportAssetParameters()
            params.set_editor_property("is_automated", True)
            if not manager.reimport_asset(asset, params):
                raise RuntimeError("Unable to reimport " + path)
            library.save_loaded_asset(asset, False)
        return asset
    task = unreal.AssetImportTask()
    task.set_editor_properties(
        {
            "filename": str((SOURCE / filename).resolve()),
            "destination_path": DEST,
            "automated": True,
            "save": True,
        }
    )
    tools.import_asset_tasks([task])
    for obj in task.get_objects():
        if isinstance(obj, (unreal.Texture2D, unreal.StaticMesh)) and obj.get_name() == name:
            return obj
    raise RuntimeError(
        "Expected " + path + "; imported " + str(task.get_editor_property("imported_object_paths"))
    )


def surface(name, file, leaves=False, normal_file=None):
    path = DEST + "/" + name
    texture = load_or_import(file)
    if library.does_asset_exist(path):
        return library.load_asset(path)
    mat = tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_properties({"used_with_instanced_static_meshes": True, "used_with_nanite": True})
    if leaves:
        mat.set_editor_property("two_sided", True)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property("opacity_mask_clip_value", 0.25)
    edit = unreal.MaterialEditingLibrary
    sample = edit.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -360, -30)
    sample.set_editor_property("texture", texture)
    edit.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    if normal_file:
        normal = load_or_import(normal_file)
        normal.set_editor_properties(
            {"compression_settings": unreal.TextureCompressionSettings.TC_NORMALMAP, "srgb": False}
        )
        library.save_loaded_asset(normal, False)
        detail = edit.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -360, 110)
        detail.set_editor_properties(
            {"texture": normal, "sampler_type": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL}
        )
        edit.connect_material_property(detail, "RGB", unreal.MaterialProperty.MP_NORMAL)
    if leaves:
        edit.connect_material_property(sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    rough = edit.create_material_expression(mat, unreal.MaterialExpressionConstant, -360, 220)
    rough.set_editor_property("r", 0.84)
    edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    library.save_loaded_asset(mat)
    return mat


def bind_meshes():
    for mesh_name, species in [
        ("SM_WesternFir", "fir_tree_01"),
        ("SM_ShorePine", "pine_tree_01"),
        ("SM_SwordFern", None),
        ("SM_NurseLog", None),
        ("SM_CreekStoneA", None),
        ("SM_CreekStoneB", None),
        ("SM_CreekStoneC", None),
    ]:
        mesh = load_or_import(mesh_name + ".glb")
        materials = {}
        if species:
            materials["Cambium"] = surface("M_" + species + "_Bark", species + "_bark_diff.jpg")
            materials["Needles"] = surface("M_" + species + "_Needles", species + "_bough.png", True)
        elif mesh_name == "SM_NurseLog":
            materials["WeatheredWood"] = surface("M_WeatheredWood", "dead_tree_trunk_02_Diffuse.jpg")
        elif mesh_name.startswith("SM_CreekStone"):
            materials["StreamStone"] = surface(
                "M_StreamStone",
                "rock_moss_set_01_Diffuse.jpg",
                normal_file="rock_moss_set_01_nor_dx.jpg",
            )
        else:
            materials["Fronds"] = surface("M_FernFronds", "FernFronds.png", True)
        for i, slot in enumerate(mesh.get_editor_property("static_materials")):
            name = str(slot.get_editor_property("material_slot_name"))
            unreal.log("CASCADES material slot " + name)
            mesh.set_material(i, materials[name])
        library.save_loaded_asset(mesh)
        unreal.log("CASCADES_IMPORTED " + mesh.get_path_name() + " " + str(mesh.get_bounds()))


if __name__ == "__main__":
    bind_meshes()
