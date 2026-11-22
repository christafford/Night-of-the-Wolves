"""Widen the old one-metre prototype doorway to 1.80 m for the return jump."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level('/Game/NightOfTheWolves/Maps/M1_Trail'):
    raise RuntimeError('Unable to open trail')
by_label = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
# Keep the -400 cm outer corner and the +170 cm window jamb in place.
for label, center_y, width in [('Cabin front left', -245, 310),
                                ('Cabin front right', 130, 80),
                                ('Cabin lintel', 0, 180),
                                ('Cabin door', 0, 180)]:
    a = by_label[label]
    p = a.get_actor_location()
    scale = a.get_actor_scale3d()
    a.set_actor_location(unreal.Vector(p.x, center_y, p.z), False, False)
    a.set_actor_scale3d(unreal.Vector(scale.x, width / 100., scale.z))
if not levels.save_current_level():
    raise RuntimeError('Doorway changes did not save')
unreal.log('FIRST LEG: doorway widened to 180 cm')

# The original M1 markers stood in the middle of the cottage yard. Retire them.
for label in ('Trail end marker left', 'Trail end marker right'):
    a = by_label.get(label)
    if a:
        a.set_actor_hidden_in_game(True)
        a.set_actor_enable_collision(False)
# Bring the old yard floor up flush with the main cottage platform. These two
# blocks flank the pit's authored floor and stop at its edge.
for label in ('Forest floor left', 'Forest floor right'):
    a = by_label.get(label)
    if a:
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, -50), False, False)
if not levels.save_current_level():
    raise RuntimeError('Unable to save the cottage yard')
