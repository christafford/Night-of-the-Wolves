"""Build an actual covered pit in the existing level with editor APIs."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
    raise RuntimeError("Could not open M1_Trail")
by_label = {actor.get_actor_label(): actor for actor in actors.get_all_level_actors()}
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
light = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")
dark = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark")

def block(label, location, scale, material, tags=(), movable=False):
    actor = by_label.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_static_mesh(cube)
    actor.static_mesh_component.set_material(0, material)
    actor.set_editor_property("tags", [unreal.Name(tag) for tag in tags])
    if movable:
        actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    return actor

# Split the trail around a 120 x 160 cm opening. The cover is removed at runtime.
block("Trail", (850, 0, -50), (27, 6, 1), light)  # x -500 to 2200
block("Trail beyond pit", (3410, 0, -50), (21.8, 6, 1), light)  # x 2320 to 4500
block("Pit side left", (2260, -190, -50), (1.2, 2.2, 1), light)
block("Pit side right", (2260, 190, -50), (1.2, 2.2, 1), light)
block("Pit cover", (2260, 0, -10), (1.2, 1.6, .2), light, ("M3PitCover",), True)
block("Pit bottom", (2260, 0, -275), (1.2, 1.6, .5), dark)

for label, blade in (("Shovel placeholder handle", False), ("Shovel placeholder blade", True)):
    actor = by_label[label]
    actor.set_editor_property("tags", [unreal.Name("M3Shovel")] + ([unreal.Name("M3Blade")] if blade else []))
    actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
by_label["Hermit placeholder"].static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
if not levels.save_current_level():
    raise RuntimeError("Could not save M1_Trail")
unreal.log("M3 covered pit and movable actors saved")
