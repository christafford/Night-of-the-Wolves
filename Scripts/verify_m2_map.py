"""Check the saved M2 blockout's key bounds and collision through editor APIs."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
    raise RuntimeError("Could not load M1_Trail")
by_label = {actor.get_actor_label(): actor for actor in actors.get_all_level_actors()}
for label in ("Trail", "Forest floor left", "Forest floor right", "Boundary far",
              "Boundary near", "Boundary left", "Boundary right", "Cabin door",
              "Hermit placeholder", "Shovel placeholder handle", "Cabin window placeholder"):
    if label not in by_label:
        raise RuntimeError("Missing " + label)
for label in ("Trail", "Forest floor left", "Forest floor right", "Boundary far",
              "Boundary near", "Boundary left", "Boundary right", "Cabin door"):
    if by_label[label].static_mesh_component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
        raise RuntimeError("Collision disabled for " + label)
origin, extent = by_label["Trail"].get_actor_bounds(False)
if origin.x - extent.x > -445:
    raise RuntimeError("Trail does not extend to the near boundary")
far_trail = by_label.get("Trail beyond pit", by_label["Trail"])
far_origin, far_extent = far_trail.get_actor_bounds(False)
if far_origin.x + far_extent.x < 4450:
    raise RuntimeError("Trail does not extend to the far boundary")
for label, outer_edge in (("Forest floor left", -2190), ("Forest floor right", 2190)):
    origin, extent = by_label[label].get_actor_bounds(False)
    if not origin.y - extent.y <= outer_edge <= origin.y + extent.y:
        raise RuntimeError(label + " does not reach its boundary")
if unreal.Name("M2Door") not in by_label["Cabin door"].get_editor_property("tags"):
    raise RuntimeError("Door tag missing")
if unreal.Name("M2Hermit") not in by_label["Hermit placeholder"].get_editor_property("tags"):
    raise RuntimeError("Hermit tag missing")
unreal.log("M2 map bounds, collision, and encounter tags verified")
