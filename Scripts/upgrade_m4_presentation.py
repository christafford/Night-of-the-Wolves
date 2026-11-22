"""Import presentation assets and dress the opening using Unreal editor APIs."""
import math
import os
import random
import unreal

ROOT = unreal.Paths.project_dir()
DEST = "/Game/NightOfTheWolves/Presentation"
MAP = "/Game/NightOfTheWolves/Maps/M1_Trail"
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
files = ["wolf_attack.png", "wolf_howls.wav", "forest_wind.wav", "forest_ground_04.jpg",
         "mud_forest.jpg", "mossy_stone_wall.jpg", "bark_willow.jpg"]
tasks = []
for name in files:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(ROOT, "Art", "Source", "Presentation", name))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    tasks.append(task)
tools.import_asset_tasks(tasks)
for name in files:
    asset_name = os.path.splitext(name)[0]
    asset = unreal.load_asset(DEST + "/" + asset_name)
    if not asset:
        raise RuntimeError("Missing imported asset " + asset_name)
    if asset_name in ("wolf_howls", "forest_wind"):
        asset.set_editor_property("looping", True)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)

def material(name, texture, tiling):
    path = DEST + "/" + name
    mat = unreal.load_asset(path)
    if mat:
        return mat
    mat = tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    sample = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -350, 0)
    sample.set_editor_property("texture", unreal.load_asset(DEST + "/" + texture))
    uv = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -600, 0)
    uv.set_editor_property("u_tiling", tiling[0])
    uv.set_editor_property("v_tiling", tiling[1])
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", sample, "UVs")
    unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 180)
    rough.set_editor_property("r", .85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat

ground = material("M_ForestGround", "forest_ground_04", (12., 4.))
trail = material("M_MoonlitTrail", "mud_forest", (10., 2.))
stone = material("M_CabinStone", "mossy_stone_wall", (3., 2.))
bark = material("M_TreeBark", "bark_willow", (1., 3.))

moon = unreal.load_asset(DEST + "/M_Moon")
if not moon:
    moon = tools.create_asset("M_Moon", DEST, unreal.Material, unreal.MaterialFactoryNew())
    moon.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(moon, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color.set_editor_property("constant", unreal.LinearColor(2.0, 2.15, 2.4, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.EditorAssetLibrary.save_loaded_asset(moon)

if not levels.load_level(MAP):
    raise RuntimeError("Cannot load " + MAP)
all_actors = actors.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
for label in ("Trail", "Trail beyond pit", "Pit side left", "Pit side right", "Pit cover"):
    by_label[label].static_mesh_component.set_material(0, trail)
for label in ("Forest floor left", "Forest floor right", "Pit bottom"):
    by_label[label].static_mesh_component.set_material(0, ground)
for label in ("Cabin front left", "Cabin front right", "Cabin lintel", "Cabin back", "Cabin left", "Cabin right", "Cabin roof"):
    by_label[label].static_mesh_component.set_material(0, stone)

cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
random.seed(1847)

def mesh_actor(label, mesh, location, scale, mat, rotation=None):
    actor = by_label.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        actor.set_actor_label(label)
        by_label[label] = actor
    actor.set_actor_location(unreal.Vector(*location), False, False)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if rotation:
        actor.set_actor_rotation(rotation, False)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, mat)
    actor.set_actor_enable_collision(False)
    return actor

# Keep the tested trunk collision actors; replace their box meshes and add high, non-colliding branches.
trees = sorted((a for a in all_actors if a.get_actor_label().startswith("Placeholder tree")), key=lambda a: a.get_actor_label())
for index, tree in enumerate(trees):
    loc = tree.get_actor_location()
    tree.static_mesh_component.set_static_mesh(cylinder)
    tree.static_mesh_component.set_material(0, bark)
    tree.set_actor_location(unreal.Vector(loc.x, loc.y, 450), False, False)
    tree.set_actor_scale3d(unreal.Vector(1.1, 1.1, 9.0))
    for n in range(4):
        azimuth = (n * 2.0 * math.pi / 4.0) + random.uniform(-.4, .4)
        start = unreal.Vector(loc.x, loc.y, 500 + n * 75)
        direction = unreal.Vector(math.cos(azimuth) * 270, math.sin(azimuth) * 270, 220 + n * 35)
        middle = start + direction * .5
        rot = unreal.MathLibrary.make_rot_from_z(direction)
        length = math.sqrt(direction.x ** 2 + direction.y ** 2 + direction.z ** 2)
        mesh_actor("M4 branch %02d %d" % (index, n), cylinder,
                   (middle.x, middle.y, middle.z), (.22, .22, length / 100.), bark, rot)

sun = by_label.get("M4 moonlight") or by_label["Placeholder moonlight"]
sun.light_component.set_light_color(unreal.LinearColor(.57, .70, 1., 1.))
sun.light_component.set_intensity(.6)
sun.set_actor_label("M4 moonlight")
skies = [a for a in all_actors if isinstance(a, unreal.SkyLight)]
if skies:
    skies[0].light_component.set_intensity(.35)
fog = by_label.get("M4 moonlit mist")
if not fog:
    fog = actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("M4 moonlit mist")
fog.component.set_fog_density(.018)
fog.component.set_fog_height_falloff(.15)
fog.component.set_fog_inscattering_color(unreal.LinearColor(.12, .19, .27, 1.))
fog.component.set_fog_max_opacity(.82)
mesh_actor("M4 full moon", sphere, (12000, -3500, 2500), (9, 9, 9), moon)
if not levels.save_current_level():
    raise RuntimeError("Cannot save " + MAP)
unreal.log("M4 presentation assets and level saved")
