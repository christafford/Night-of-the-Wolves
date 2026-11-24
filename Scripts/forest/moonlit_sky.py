"""Set the forest sky, full-moon direction and diffuse ambient for the playable night."""

import math
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
# In the panorama the moon is at U=.598 and V=.418. Keeping the horizon
# level while compressing the sampled elevation raises the moon above the canopy.
panorama_offset = .80
moon_yaw = (.598 - panorama_offset) * 360
moon_pitch = math.degrees(math.atan(math.tan((.5 - .418) * math.pi) / .50))
moon_x = math.cos(math.radians(moon_pitch)) * math.cos(math.radians(moon_yaw))
moon_y = math.cos(math.radians(moon_pitch)) * math.sin(math.radians(moon_yaw))
moon_z = math.sin(math.radians(moon_pitch))

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
        "code": ("float3 d=normalize(-V); d.z *= .50; d=normalize(d); "
                 f"return float2(atan2(d.y,d.x)*0.15915494+{panorama_offset},acos(clamp(d.z,-1,1))*0.31830989);"),
    }
)
edit.connect_material_expressions(direction, "", coords, "V")
sample = edit.create_material_expression(mat, u.MaterialExpressionTextureSample, -370, 0)
sample.set_editor_property("texture", tex)
edit.connect_material_expressions(coords, "", sample, "UVs")
# Keep the photograph's stars and halo but suppress the broad band of
# warm dusk. Resolve a round disk inside the photographed overexposed core.
grade = edit.create_material_expression(mat, u.MaterialExpressionCustom, -100, 0)
color_input = u.CustomInput()
color_input.set_editor_property("input_name", "Sky")
grade.set_editor_properties({
    "inputs": [param, color_input],
    "output_type": u.CustomMaterialOutputType.CMOT_FLOAT3,
    "description": "Midnight air, reflected moon halo and full lunar disk",
    "code": f"""float3 d = normalize(-V);
float3 moonDir = float3({moon_x:.8f}, {moon_y:.8f}, {moon_z:.8f});
float dist = length(d - moonDir);
float altitude = smoothstep(-0.06, 0.40, d.z);
float luminance = dot(Sky, float3(.2126, .7152, .0722));
float3 sky = lerp(luminance.xxx, Sky, .27) * float3(.68, .83, 1.) * lerp(.038, .102, altitude);
float aa = clamp(fwidth(dist), .0005, .004);
float disk = 1. - smoothstep(.0075 - aa, .0075 + aa, dist);
float halo = exp(-dist * 53.);
return sky + float3(.78, .86, 1.) * halo * .14 + float3(.95, .97, 1.) * disk * 2.4;""",
})
edit.connect_material_expressions(direction, "", grade, "V")
edit.connect_material_expressions(sample, "RGB", grade, "Sky")
edit.connect_material_property(grade, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
lib.save_loaded_asset(mat, False)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail")
by = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
moon = by["M4 moonlight"]
moon.set_actor_rotation(u.Rotator(pitch=-moon_pitch, yaw=moon_yaw+180, roll=0), False)
moon.light_component.set_editor_property("forward_shading_priority", 1)
moon.light_component.set_editor_property("light_source_angle", .86)
moon.light_component.set_light_color(u.LinearColor(.75, .82, 1., 1.))
moon.light_component.set_intensity(.35)
# The sky supplies the forest's diffuse ambient. A second directional source
# would fill back-facing trunks and cast light right through the closed canopy.
if "Cascades diffuse sky fill" in by:
    actors.destroy_actor(by["Cascades diffuse sky fill"])
skylight = next(a.light_component for a in by.values() if isinstance(a, u.SkyLight))
# Sample diffuse air above the whole forest, away from the enclosing trees;
# the visible sky dome is independently exposed for the eye. Unreal shades
# this hemisphere with Lumen sky visibility in the interior and under crowns.
skylight.set_mobility(u.ComponentMobility.MOVABLE)
skylight.set_editor_property("source_type", u.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
diffuse_cube = u.load_object(None, "/Engine/EngineResources/GrayTextureCube.GrayTextureCube")
if not isinstance(diffuse_cube, u.TextureCube):
    raise RuntimeError("Missing diffuse sky cubemap")
skylight.set_cubemap(diffuse_cube)
skylight.set_light_color(u.LinearColor(.55, .68, .88, 1.))
skylight.set_intensity(.47)
skylight.set_editor_property("lower_hemisphere_color", u.LinearColor(.010, .014, .021, 1.))
fog = by["M4 moonlit mist"].component
fog.set_fog_density(.026)
fog.set_fog_height_falloff(.16)
fog.set_fog_inscattering_color(u.LinearColor(.009, .015, .024, 1.))
fog.set_fog_max_opacity(.85)
fog.set_volumetric_fog(True)
fog.set_volumetric_fog_distance(7200.)
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
skylight.recapture_sky()
if not levels.save_current_level():
    raise RuntimeError("Failed to save sky")
u.log("MOONRISE_SAVED")
