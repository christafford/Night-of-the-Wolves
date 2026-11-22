"""Place a distant photographed night sky with the moon ahead of the opening trail."""

from pathlib import Path
import unreal as u

dest = "/Game/NightOfTheWolves/Cascades"
lib = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
if not lib.does_asset_exist(dest + "/MoonriseSky"):
    task = u.AssetImportTask()
    source = Path(u.Paths.project_dir()) / "Art/Source/Cascades/MoonriseSky.jpg"
    task.set_editor_properties(
        {"filename": str(source.resolve()), "destination_path": dest, "automated": True, "save": True}
    )
    tools.import_asset_tasks([task])
tex = lib.load_asset(dest + "/MoonriseSky")
name = "M_MoonriseSky"
if lib.does_asset_exist(dest + "/" + name):
    mat = lib.load_asset(dest + "/" + name)
else:
    mat = tools.create_asset(name, dest, u.Material, u.MaterialFactoryNew())
mat.set_editor_properties(
    {
        "shading_model": u.MaterialShadingModel.MSM_UNLIT,
        "two_sided": True,
        "is_sky": True,
        "used_with_nanite": True,
    }
)
edit = u.MaterialEditingLibrary
edit.delete_all_material_expressions(mat)
direction = edit.create_material_expression(mat, u.MaterialExpressionCameraVectorWS, -900, 0)
coords = edit.create_material_expression(mat, u.MaterialExpressionCustom, -600, 0)
param = u.CustomInput()
param.set_editor_property("input_name", "V")
coords.set_editor_properties(
    {
        "inputs": [param],
        "output_type": u.CustomMaterialOutputType.CMOT_FLOAT2,
        "description": "Moonrise panorama coordinates",
        "code": "float3 d=normalize(-V); return float2(atan2(d.y,d.x)*0.15915494+0.60,acos(clamp(d.z,-1,1))*0.31830989);",
    }
)
edit.connect_material_expressions(direction, "", coords, "V")
sample = edit.create_material_expression(mat, u.MaterialExpressionTextureSample, -370, 0)
sample.set_editor_property("texture", tex)
edit.connect_material_expressions(coords, "", sample, "UVs")
intensity = edit.create_material_expression(mat, u.MaterialExpressionMultiply, -100, 0)
intensity.set_editor_property("const_b", 0.37)
edit.connect_material_expressions(sample, "RGB", intensity, "A")
edit.connect_material_property(intensity, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
lib.save_loaded_asset(mat, False)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail")
by = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
by["M4 moonlight"].light_component.set_editor_property("forward_shading_priority", 1)
sky = by.get("Cascades moonrise sky") or actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
sky.set_actor_label("Cascades moonrise sky")
sky.set_actor_scale3d(u.Vector(5200, 5200, 5200))
sky.static_mesh_component.set_static_mesh(lib.load_asset("/Engine/BasicShapes/Sphere"))
sky.static_mesh_component.set_material(0, mat)
sky.static_mesh_component.set_editor_properties(
    {"cast_shadow": False, "affect_distance_field_lighting": False, "visible_in_ray_tracing": False}
)
sky.set_actor_enable_collision(False)
by["M4 full moon"].set_actor_hidden_in_game(True)
by["M4 full moon"].set_actor_enable_collision(False)
if not levels.save_current_level():
    raise RuntimeError("Failed to save sky")
u.log("MOONRISE_SAVED")
