"""Extend the existing forest floor under the outer blockout/PCG tree rows."""
import unreal
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level('/Game/NightOfTheWolves/Maps/M1_Trail'):
    raise RuntimeError('Unable to load trail map')
count = 0
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith('M5 forest floor '):
        scale = actor.get_actor_scale3d()
        scale.y = 32.0
        actor.set_actor_scale3d(scale)
        count += 1
if not count or not levels.save_current_level():
    raise RuntimeError('Failed to extend and save forest floor')
unreal.log('FOREST_GROUND_EXTENDED ' + str(count))
