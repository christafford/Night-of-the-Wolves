"""Build the longer approach and fireplace using Unreal Editor APIs."""
import math
import random
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
    raise RuntimeError("Could not open the opening map")
all_actors = actors.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
cone = unreal.load_asset("/Engine/BasicShapes/Cone.Cone")
trail_mat = unreal.load_asset("/Game/NightOfTheWolves/Presentation/M_MoonlitTrail")
ground_mat = unreal.load_asset("/Game/NightOfTheWolves/Presentation/M_ForestGround")
bark = unreal.load_asset("/Game/NightOfTheWolves/Presentation/M_TreeBark")
stone = unreal.load_asset("/Game/NightOfTheWolves/Presentation/M_CabinStone")

def block(label, mesh, position, scale, material, rotation=None, collision=True):
    actor = by_label.get(label)
    if not actor:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position))
        actor.set_actor_label(label)
        by_label[label] = actor
    actor.set_actor_location(unreal.Vector(*position), False, False)
    actor.set_actor_rotation(rotation or unreal.Rotator(0, 0, 0), False)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, material)
    actor.set_actor_enable_collision(collision)
    return actor

# Smooth, non-intersecting approach; the final straight joins the existing cabin puzzle.
control = [(-14000, -3700, 0), (-12500, -3700, 80), (-10800, -2800, 180),
           (-9000, -2800, 40), (-7500, -3800, -80), (-5700, -3700, 100),
           (-3900, -2300, 210), (-2200, -900, 120), (-900, 0, 0), (0, 0, 0), (3000, 0, 0)]

def catmull(a, b, c, d, t):
    return tuple(.5 * ((2 * b[i]) + (-a[i] + c[i]) * t +
                 (2*a[i] - 5*b[i] + 4*c[i] - d[i]) * t*t +
                 (-a[i] + 3*b[i] - 3*c[i] + d[i]) * t*t*t) for i in range(3))

points = []
for i in range(len(control) - 1):
    a, b, c, d = control[max(i-1, 0)], control[i], control[i+1], control[min(i+2, len(control)-1)]
    length = math.dist(b, c)
    steps = max(1, math.ceil(length / 300))
    for j in range(steps):
        points.append(catmull(a, b, c, d, j / steps))
points.append(control[-1])

route = by_label.get("M5 pursuit route")
if not route:
    route = actors.spawn_actor_from_class(unreal.WolfTrailRoute, unreal.Vector(0, 0, 0))
    route.set_actor_label("M5 pursuit route")
route.spline.clear_spline_points(False)
for point in points:
    route.spline.add_spline_point(unreal.Vector(*point), unreal.SplineCoordinateSpace.LOCAL, False)
for index in range(len(points)):
    route.spline.set_spline_point_type(index, unreal.SplinePointType.LINEAR, False)
route.spline.update_spline()

start = by_label["Max start - facing trail"]
start.set_actor_location(unreal.Vector(control[0][0], control[0][1], control[0][2] + 100), False, False)
start.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
# Remove the old near-end wall's collision; it stood across the new approach.
by_label["Boundary near"].set_actor_enable_collision(False)

random.seed(9723)
# Extend the ground behind Max so the initial third-person camera doesn't look over a platform edge.
block("M5 start ground", cube, (-14200, -3700, -50), (6, 8, 1), trail_mat)
for i, (a, b) in enumerate(zip(points, points[1:])):
    # The old cabin area already has a split floor and physical pit; leave it intact.
    if a[0] >= -510:
        break
    direction = unreal.Vector(b[0]-a[0], b[1]-a[1], b[2]-a[2])
    length = math.sqrt(direction.x**2 + direction.y**2 + direction.z**2)
    if length < 1:
        continue
    midpoint = ((a[0]+b[0])*.5, (a[1]+b[1])*.5, (a[2]+b[2])*.5 - 50)
    rotation = unreal.MathLibrary.make_rot_from_x(direction)
    block("M5 path %03d" % i, cube, midpoint, ((length + 45)/100., 8., 1.), trail_mat, rotation)
    horizontal = math.hypot(direction.x, direction.y)
    sideways = unreal.Vector(-direction.y / horizontal, direction.x / horizontal, 0)
    for side in (-1, 1):
        center = unreal.Vector(*midpoint) + sideways * (side * 900)
        block("M5 forest floor %03d %d" % (i, side), cube, (center.x, center.y, center.z - 8),
              ((length + 50)/100., 10., 1.), ground_mat, rotation)
        # Keep one readable path, with trees outside its walkable width.
        if i % 3 == 0:
            x = (a[0]+b[0])*.5 + sideways.x * side * random.uniform(550, 750)
            y = (a[1]+b[1])*.5 + sideways.y * side * random.uniform(550, 750)
            z = (a[2]+b[2])*.5
            height = random.uniform(700, 1050)
            block("M5 tree %03d %d" % (i, side), cylinder, (x, y, z + height/2), (1.1, 1.1, height/100), bark)
            for n in range(3):
                angle = random.uniform(0, 2*math.pi)
                direction2 = unreal.Vector(math.cos(angle)*random.uniform(160, 260), math.sin(angle)*random.uniform(160, 260), random.uniform(130, 250))
                origin = unreal.Vector(x, y, z + height*(.55+n*.12))
                mid = origin + direction2*.5
                branch_len = math.sqrt(direction2.x**2 + direction2.y**2 + direction2.z**2)
                block("M5 branch %03d %d %d" % (i, side, n), cylinder, (mid.x, mid.y, mid.z),
                      (.18, .18, branch_len/100), bark, unreal.MathLibrary.make_rot_from_z(direction2), False)

# A low forested ridge screens the cabin from the start but stays well away from the trail.
for index, (x, y, z, sx, sy, sz) in enumerate([(-6000, -1900, 480, 28, 16, 13), (-6600, -1200, 410, 22, 14, 11), (-5300, -2100, 360, 18, 12, 10)]):
    block("M5 distant wooded ridge %d" % index, sphere, (x, y, z), (sx, sy, sz), ground_mat, collision=False)

# Keep the player in the world if they stray from the narrow prototype road.
block("M5 underforest safety floor", cube, (-7000, -2800, -450), (160, 80, 1), ground_mat)

destination = "/Game/NightOfTheWolves/Presentation"
fire_mat = unreal.load_asset(destination + "/M_FireGlow")
if not fire_mat:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    fire_mat = asset_tools.create_asset("M_FireGlow", destination, unreal.Material, unreal.MaterialFactoryNew())
    fire_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(fire_mat, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color.set_editor_property("constant", unreal.LinearColor(2.0, .3, .025, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.EditorAssetLibrary.save_loaded_asset(fire_mat)
else:
    for expression in unreal.MaterialEditingLibrary.get_material_expressions(fire_mat):
        if isinstance(expression, unreal.MaterialExpressionConstant3Vector):
            expression.set_editor_property("constant", unreal.LinearColor(2.0, .3, .025, 1))
    unreal.EditorAssetLibrary.save_loaded_asset(fire_mat)

# Replace the painted window with an actual opening in the front wall.
by_label["Cabin window placeholder"].set_actor_hidden_in_game(True)
by_label["Cabin window placeholder"].set_actor_enable_collision(False)
block("Cabin front right", cube, (2450, 110, 150), (1, 1.2, 3), stone)
block("M5 window wall right", cube, (2450, 335, 150), (1, 1.3, 3), stone)
block("M5 window sill wall", cube, (2450, 220, 70), (1, 1, 1.4), stone)
block("M5 window lintel wall", cube, (2450, 220, 270), (1, 1, .6), stone)
# Interior hearth is opposite the opening so the flame can be seen through it.
block("M5 fireplace back", cube, (3160, 220, 125), (1, 2, 2.5), stone)
block("M5 fireplace hearth", cube, (3065, 220, 15), (2.5, 2.5, .3), stone)
for i, (dx, dy, scale) in enumerate([(-25, -20, .55), (5, 10, .7), (30, -15, .45)]):
    block("M5 flame %d" % i, cone, (3065+dx, 220+dy, 72), (.3, .3, scale), fire_mat, collision=False)

def point_light(label, location, intensity, radius, tag):
    light = by_label.get(label)
    if not light:
        light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*location))
        light.set_actor_label(label)
    light.set_actor_location(unreal.Vector(*location), False, False)
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.light_component.set_light_color(unreal.LinearColor(1.0, .28, .035, 1))
    light.light_component.set_intensity(intensity)
    light.light_component.set_attenuation_radius(radius)
    light.set_editor_property("tags", [unreal.Name(tag)])

point_light("M5 hearth firelight", (3050, 220, 145), 30, 850, "M5FireLight")
# A matching exterior spill makes the small fire readable from the approach.
point_light("M5 window firelight spill", (2350, 220, 195), 12, 450, "M5WindowLight")
if not levels.save_current_level():
    raise RuntimeError("Could not save opening map")
unreal.log("M5 winding approach, pursuit route, window and hearth saved: %d samples" % len(points))
