"""Update the existing blockout in-place using Unreal Editor APIs."""
import unreal

MAP = "/Game/NightOfTheWolves/Maps/M1_Trail"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level(MAP):
    raise RuntimeError("Could not open " + MAP)
all_actors = actors.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
light = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")
dark = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark")

def block(label, location, scale, material, tag=None, hidden=False):
    actor = by_label.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    actor.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_static_mesh(cube)
    actor.static_mesh_component.set_material(0, material)
    actor.set_editor_property("tags", [unreal.Name(tag)] if tag else [])
    actor.set_actor_hidden_in_game(hidden)
    if hidden:
        actor.set_actor_enable_collision(False)
    return actor

# Extend the playable floor and enclose its perimeter so the player cannot walk off it.
block("Trail", (2000, 0, -50), (50, 6, 1), light)
block("Forest floor left", (2000, -1250, -75), (50, 19, 1), dark)
block("Forest floor right", (2000, 1250, -75), (50, 19, 1), dark)
block("Boundary far", (4450, 0, 150), (1, 44, 3), dark)
block("Boundary near", (-450, 0, 150), (1, 44, 3), dark)
block("Boundary left", (2000, -2190, 150), (50, 1, 3), dark)
block("Boundary right", (2000, 2190, 150), (50, 1, 3), dark)

# Cabin front faces the trail. A separate door and window are placeholders.
block("Cabin front left", (2450, -225, 150), (1, 3.5, 3), dark)
block("Cabin front right", (2450, 225, 150), (1, 3.5, 3), dark)
block("Cabin lintel", (2450, 0, 275), (1, 1, .5), dark)
block("Cabin back", (3250, 0, 150), (1, 8, 3), dark)
block("Cabin left", (2850, -400, 150), (8, 1, 3), dark)
block("Cabin right", (2850, 400, 150), (8, 1, 3), dark)
block("Cabin roof", (2850, 0, 325), (8.2, 8.2, .5), dark)
door = block("Cabin door", (2450, 0, 125), (.3, 1, 2.5), light, "M2Door")
door.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
block("Cabin window placeholder", (2435, 220, 190), (.2, 1, 1), light)
block("Shovel placeholder handle", (2390, 310, 65), (.12, .12, 1.3), light)
block("Shovel placeholder blade", (2390, 310, 10), (.3, .35, .2), light)
block("Hermit placeholder", (2570, 0, 100), (.6, .6, 2), light, "M2Hermit", True)
if not levels.save_current_level():
    raise RuntimeError("Could not save " + MAP)
unreal.log("M2 blockout saved: " + MAP)
