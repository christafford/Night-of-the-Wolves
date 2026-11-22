"""Plant overlapping stand silhouettes across the ridge behind the cottage clearing."""

import bisect
import math
import random
import unreal as u

D = "/Game/NightOfTheWolves/Cascades"
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail")
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
spline = next(
    a for a in actors.get_all_level_actors() if a.get_actor_label() == "M5 pursuit route"
).spline
space = u.SplineCoordinateSpace.WORLD
p = [
    spline.get_location_at_distance_along_spline(d, space)
    for d in range(0, int(spline.get_spline_length()), 95)
]
xs = [v.x for v in p]


def elevation(x, y):
    i = max(0, min(len(p) - 2, bisect.bisect_right(xs, x) - 1))
    a, b = p[i : i + 2]
    dx = max(1, b.x - a.x)
    t = max(0, min(1, (x - a.x) / dx))
    off = (y - (a.y + t * (b.y - a.y))) / math.hypot(1, (b.y - a.y) / dx)
    start = 370 + 1950 * max(0.0, 1 - max(0.0, abs(x - 2400.0) - 1000.0) / 1050.0)
    ramp = max(0, min(1, (off - start) / 2400))
    roll = 95 * math.sin(x * 0.00084 + 0.9) + 75 * math.sin(x * 0.00197 + off * 0.0017)
    return (
        a.z
        + t * (b.z - a.z)
        - 21
        + ramp * (570 + roll + 90 * math.sin(off * 0.0023 + x * 0.0009))
        + max(0, off - 3000) * 0.06
    )


rng = random.Random(71489)
fir = []
pine = []
for y in (1850, 2560, 3560, 4760):
    for sx in range(-1100, 8500, 400):
        x = sx + rng.uniform(-195, 195)
        offset = y + rng.uniform(-240, 240)
        tree = fir if rng.random() < 0.66 else pine
        s = rng.uniform(0.55, 1.40)
        tree.append(
            u.Transform(
                location=(x, offset, elevation(x, offset) - 12),
                rotation=u.Rotator(yaw=rng.uniform(0, 360)),
                scale=(s, s, s),
            )
        )
for name, geo, transforms in [
    ("FT_CottageRidgeFir", "SM_WesternFir", fir),
    ("FT_CottageRidgePine", "SM_ShorePine", pine),
]:
    lib = u.EditorAssetLibrary
    if lib.does_asset_exist(D + "/" + name):
        f = lib.load_asset(D + "/" + name)
    else:
        f = u.AssetToolsHelpers.get_asset_tools().create_asset(
            name, D, u.FoliageType_InstancedStaticMesh, u.FoliageType_InstancedStaticMeshFactory()
        )
    f.set_editor_properties(
        {
            "mesh": lib.load_asset(D + "/" + geo + "/StaticMeshes/" + geo),
            "cull_distance": u.Int32Interval(16000, 22000),
        }
    )
    lib.save_loaded_asset(f)
    u.InstancedFoliageActor.remove_all_instances(world, f)
    u.InstancedFoliageActor.add_instances_and_wait(world, f, transforms)
if not levels.save_current_level():
    raise RuntimeError("Cannot save the ridge foliage")
u.log("RIDGE_SAVED " + str(len(pine) + len(fir)))
