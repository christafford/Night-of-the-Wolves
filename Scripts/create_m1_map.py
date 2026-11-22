"""Create the M1 blockout using editor APIs. This script is not used at runtime."""
import unreal

MAP = "/Game/NightOfTheWolves/Maps/M1_Trail"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError("M1 map already exists; refusing to overwrite it")
if not levels.new_level(MAP):
    raise RuntimeError("Could not create M1 map")

world = unreal.EditorLevelLibrary.get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.WolfTrailGameMode)
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
trail_mat = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")
dark_mat = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark")

def block(label, location, scale, material):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if material:
        actor.static_mesh_component.set_material(0, material)
    return actor

# Cube units are 100 cm. The pale trail runs along +X and extends beyond its marker.
block("Trail", (1200, 0, -50), (34, 6, 1), trail_mat)
block("Forest floor left", (1200, -1100, -75), (34, 16, 1), dark_mat)
block("Forest floor right", (1200, 1100, -75), (34, 16, 1), dark_mat)
for x in range(-300, 3001, 400):
    for y in (-520, 520):
        block("Placeholder tree", (x, y, 200), (1.2, 1.2, 4), dark_mat)
block("Trail end marker left", (2000, -250, 125), (0.5, 0.5, 2.5), trail_mat)
block("Trail end marker right", (2000, 250, 125), (0.5, 0.5, 2.5), trail_mat)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100), unreal.Rotator(0, 0, 0))
start.set_actor_label("Max start - facing trail")
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-45, -30, 0))
sun.set_actor_label("Placeholder moonlight")
sun.light_component.set_editor_property("intensity", 3.0)
actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))
if not levels.save_current_level():
    raise RuntimeError("Could not save M1 map")
unreal.log("M1 map created and saved: " + MAP)
