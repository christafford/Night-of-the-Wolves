"""Shorten the opening, dress the trail, and put the cabin beside a continuing path."""
import math
import os
import random
import unreal

MAP = "/Game/NightOfTheWolves/Maps/M1_Trail"
DEST = "/Game/NightOfTheWolves/Presentation"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

task = unreal.AssetImportTask()
task.set_editor_property("filename", os.path.join(unreal.Paths.project_dir(), "Art/Source/Presentation/grey_stone_path.jpg"))
task.set_editor_property("destination_path", DEST)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("replace_existing", True)
asset_tools.import_asset_tasks([task])
texture = unreal.load_asset(DEST + "/grey_stone_path")
if not texture:
    raise RuntimeError("Could not import stone path texture")

def texture_material(name, tex):
    result = unreal.load_asset(DEST + "/" + name)
    if not result:
        result = asset_tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
        sample = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionTextureSample, -350, 0)
        sample.set_editor_property("texture", tex)
        uv = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionTextureCoordinate, -600, 0)
        uv.set_editor_property("u_tiling", 2.0)
        uv.set_editor_property("v_tiling", 2.0)
        unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", sample, "UVs")
        unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant, -250, 200)
        rough.set_editor_property("r", .9)
        unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.EditorAssetLibrary.save_loaded_asset(result)
    else:
        sample = unreal.MaterialEditingLibrary.get_material_property_input_node(result, unreal.MaterialProperty.MP_BASE_COLOR)
        if not sample:
            raise RuntimeError("Path material has no texture sample")
        sample.set_editor_property("texture", tex)
        unreal.EditorAssetLibrary.save_loaded_asset(result)
    return result

def color_material(name, value):
    result = unreal.load_asset(DEST + "/" + name)
    if not result:
        result = asset_tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
        color = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector, -300, 0)
        color.set_editor_property("constant", unreal.LinearColor(*value, 1))
        unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.EditorAssetLibrary.save_loaded_asset(result)
    return result

path_mat = texture_material("M_MossyPath", texture)
foliage = color_material("M_DarkFir", (.022, .055, .035))
if not levels.load_level(MAP):
    raise RuntimeError("Could not load map")
all_actors = actors.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
cone = unreal.load_asset("/Engine/BasicShapes/Cone.Cone")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
ground = unreal.load_asset(DEST + "/M_ForestGround")
bark = unreal.load_asset(DEST + "/M_TreeBark")

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
    actor.set_actor_hidden_in_game(False)
    actor.set_actor_enable_collision(collision)
    return actor

# Clear the previous road and trees before rebuilding, including any obsolete segments.
for actor in all_actors:
    label = actor.get_actor_label()
    if label.startswith(("M5 path ", "M5 forest floor ", "M5 tree ", "M5 branch ",
                         "M4 organic ", "M4 branch ", "M6 forest ", "M6 stone ", "M6 path ")):
        actor.set_actor_hidden_in_game(True)
        actor.set_actor_enable_collision(False)
    if label.startswith("Placeholder tree"):
        actor.set_actor_hidden_in_game(True)
        actor.set_actor_enable_collision(False)

control = [(-14000, -3700, 0), (-12500, -3700, 80), (-10800, -2800, 180),
           (-9000, -2800, 40), (-7500, -3800, -80), (-5700, -3700, 100),
           (-3900, -2300, 210), (-2200, -900, 120), (-900, -900, 0),
           (0, -900, 0), (1800, -900, 0), (4000, -1050, 20), (6500, -900, 60), (8500, -1200, 50)]

def catmull(a, b, c, d, t):
    return tuple(.5*((2*b[k])+(-a[k]+c[k])*t+(2*a[k]-5*b[k]+4*c[k]-d[k])*t*t+
                    (-a[k]+3*b[k]-3*c[k]+d[k])*t*t*t) for k in range(3))

points = []
for i in range(len(control)-1):
    a, b, c, d = control[max(i-1, 0)], control[i], control[i+1], control[min(i+2, len(control)-1)]
    steps = max(1, math.ceil(math.dist(b, c)/300))
    for j in range(steps):
        points.append(catmull(a, b, c, d, j/steps))
points.append(control[-1])
route = by_label["M5 pursuit route"]
route.spline.clear_spline_points(False)
for p in points:
    route.spline.add_spline_point(unreal.Vector(*p), unreal.SplineCoordinateSpace.LOCAL, False)
for i in range(len(points)):
    route.spline.set_spline_point_type(i, unreal.SplinePointType.LINEAR, False)
route.spline.update_spline()

# Spawn near the middle of the already-built route, facing its forward tangent.
start = by_label["Max start - facing trail"]
start.set_actor_location(unreal.Vector(-7500, -3800, 20), False, False)
start.set_actor_rotation(unreal.Rotator(0, math.degrees(math.atan2(-900, 3300)), 0), False)
by_label["Boundary near"].set_actor_enable_collision(False)
by_label["Boundary far"].set_actor_location(unreal.Vector(9000, -500, 150), False, False)

# The old path in front of the cabin becomes forest ground; keep the physical pit and cover intact.
for label in ("Trail", "Trail beyond pit", "Pit side left", "Pit side right", "Pit cover"):
    by_label[label].static_mesh_component.set_material(0, ground)

random.seed(61411)
for i, (a, b) in enumerate(zip(points, points[1:])):
    direction = unreal.Vector(b[0]-a[0], b[1]-a[1], b[2]-a[2])
    length = math.sqrt(direction.x**2 + direction.y**2 + direction.z**2)
    horizontal = math.hypot(direction.x, direction.y)
    if horizontal < 1:
        continue
    side = unreal.Vector(-direction.y/horizontal, direction.x/horizontal, 0)
    mid = unreal.Vector((a[0]+b[0])*.5, (a[1]+b[1])*.5, (a[2]+b[2])*.5)
    rotation = unreal.MathLibrary.make_rot_from_x(direction)
    block("M6 path %03d" % i, cube, (mid.x, mid.y, mid.z-50), ((length+45)/100, 7.5, 1), path_mat, rotation)
    # Behind the cabin the old level has its own floor and pit; do not place floor over them.
    if mid.x < -500 or mid.x > 4500:
        for sign in (-1, 1):
            center = mid + side * (sign * 875)
            block("M5 forest floor %03d %d" % (i, sign), cube, (center.x, center.y, center.z-58),
                  ((length+50)/100, 32., 1), ground, rotation)
    # Small, partly embedded rocks soften the edges without interfering with movement.
    for n in range(3):
        along = random.uniform(-.35, .35)
        offset = random.choice((-1, 1)) * random.uniform(230, 360)
        p = mid + direction * along + side * offset
        block("M6 stone %03d %d" % (i, n), sphere, (p.x, p.y, p.z-1),
              (random.uniform(.35, .8), random.uniform(.3, .65), random.uniform(.05, .10)), path_mat, collision=False)
    if i % 2:
        continue
    for sign in (-1, 1):
        # Leave an obvious opening to the cabin on the right side of the main route.
        if sign == 1 and 1050 < mid.x < 2850:
            continue
        for row in (0, 1, 2):
            offset = (random.uniform(500, 680) if row == 0 else
                      random.uniform(950, 1450) if row == 1 else random.uniform(1550, 2100)) * sign
            p = mid + side*offset + direction*random.uniform(-.35, .35)
            height = random.uniform(650, 1050)
            label = "M6 forest %03d %d %d" % (i, sign, row)
            # Alternate firs and bare woodland trees, varying size and shape.
            fir = (i//2 + row + (1 if sign > 0 else 0)) % 3 != 0
            block(label+" trunk", cylinder, (p.x, p.y, p.z+height*.5),
                  (random.uniform(.65, 1.15), random.uniform(.65, 1.15), height/100), bark, collision=(row == 0))
            if fir:
                for tier in range(3):
                    width = (3.3-tier*.55) * random.uniform(.85, 1.15)
                    block(label+" fir %d" % tier, cone, (p.x, p.y, p.z+height*(.52+tier*.16)),
                          (width, width, 3.8-tier*.35), foliage, collision=False)
            else:
                for n in range(4):
                    angle = random.uniform(0, 2*math.pi)
                    v = unreal.Vector(math.cos(angle)*random.uniform(150, 250), math.sin(angle)*random.uniform(150, 250), random.uniform(120, 250))
                    origin = unreal.Vector(p.x, p.y, p.z+height*(.5+n*.12))
                    center = origin+v*.5
                    branch_length = math.sqrt(v.x*v.x+v.y*v.y+v.z*v.z)
                    block(label+" branch %d" % n, cylinder, (center.x, center.y, center.z),
                          (.16, .16, branch_length/100), bark, unreal.MathLibrary.make_rot_from_z(v), False)

# A rough side trail leaves the through-route and approaches the cabin and shovel.
block("M6 cabin spur", cube, (1900, -500, -50), (4.5, 8.5, 1), path_mat)
# Continue the terrain under and around the new far section.
block("M6 far forest floor", cube, (6500, -900, -80), (50, 30, 1), ground)
if not levels.save_current_level():
    raise RuntimeError("Could not save revised opening")
unreal.log("M6 forest, middle start, side cabin and continuing trail saved: %d route samples" % len(points))
