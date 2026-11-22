"""Replace the first-pass visible tree shapes with varied branching prototypes."""
import math
import random
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
    raise RuntimeError("Could not load level")
all_actors = actors.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
trunks = sorted((a for a in all_actors if a.get_actor_label().startswith("Placeholder tree")), key=lambda a: a.get_actor_label())
for actor in all_actors:
    label = actor.get_actor_label()
    if label.startswith("M4 branch ") or label.startswith("Boundary "):
        actor.set_actor_hidden_in_game(True)
for trunk in trunks:
    # Preserve the tested physical collision; only replace its appearance.
    trunk.set_actor_hidden_in_game(True)

cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
bark = unreal.load_asset("/Game/NightOfTheWolves/Presentation/M_TreeBark")
random.seed(4793)

def segment(label, a, b, radius):
    direction = b - a
    length = math.sqrt(direction.x ** 2 + direction.y ** 2 + direction.z ** 2)
    mid = a + direction * .5
    actor = by_label.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, mid)
        actor.set_actor_label(label)
        by_label[label] = actor
    actor.set_actor_location(mid, False, False)
    actor.set_actor_rotation(unreal.MathLibrary.make_rot_from_z(direction), False)
    actor.set_actor_scale3d(unreal.Vector(radius, radius, length / 100.))
    actor.static_mesh_component.set_static_mesh(cylinder)
    actor.static_mesh_component.set_material(0, bark)
    actor.set_actor_enable_collision(False)
    actor.set_actor_hidden_in_game(False)

for index, trunk in enumerate(trunks):
    base = trunk.get_actor_location()
    start = unreal.Vector(base.x, base.y, 0)
    height = random.uniform(650, 1050)
    tilt_x, tilt_y = random.uniform(-100, 100), random.uniform(-95, 95)
    points = [start]
    for n in range(1, 6):
        fraction = n / 5.
        points.append(unreal.Vector(base.x + tilt_x * fraction + 16 * math.sin(fraction * 4 + index),
                                    base.y + tilt_y * fraction + 14 * math.sin(fraction * 6 + index), height * fraction))
        segment("M4 organic trunk %02d %d" % (index, n), points[n-1], points[n], 1.0 - n * .13)
    for n in range(2, 6):
        origin = points[n]
        angle = random.uniform(0, 2 * math.pi)
        distance = random.uniform(180, 300)
        elbow = origin + unreal.Vector(math.cos(angle) * distance * .55, math.sin(angle) * distance * .55, random.uniform(70, 150))
        tip = elbow + unreal.Vector(math.cos(angle + .3) * distance * .55, math.sin(angle + .3) * distance * .55, random.uniform(60, 150))
        segment("M4 organic limb %02d %d a" % (index, n), origin, elbow, .22)
        segment("M4 organic limb %02d %d b" % (index, n), elbow, tip, .12)
        fork = elbow + unreal.Vector(math.cos(angle - .7) * distance * .6, math.sin(angle - .7) * distance * .6, random.uniform(60, 160))
        segment("M4 organic fork %02d %d" % (index, n), elbow, fork, .09)

moon = by_label["M4 full moon"]
moon.set_actor_location(unreal.Vector(4000, -900, 1100), False, False)
moon.set_actor_scale3d(unreal.Vector(4, 4, 4))
fog = by_label["M4 moonlit mist"]
fog.component.set_fog_inscattering_color(unreal.LinearColor(.045, .07, .11, 1))
if not levels.save_current_level():
    raise RuntimeError("Could not save forest")
unreal.log("M4 forest shapes refined; tested collision preserved")
