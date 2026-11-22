"""Author the Cascades stands in the *full editor* so foliage simulation has a physics scene."""

import bisect
import math
import random
import unreal as u

DEST = "/Game/NightOfTheWolves/Cascades"
PRESENT = "/Game/NightOfTheWolves/Presentation"
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
worlds = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
    raise RuntimeError("Cannot load trail map")
world = worlds.get_editor_world()
by = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
spline = by["M5 pursuit route"].spline
space = u.SplineCoordinateSpace.WORLD
length = spline.get_spline_length()
rand = random.Random(19037)
litter = assets.load_asset(DEST + "/M_NeedleLitter")
tread = assets.load_asset(DEST + "/M_DampTread")


def get(name, cls, factory):
    path = DEST + "/" + name
    return (
        assets.load_asset(path)
        if assets.does_asset_exist(path)
        else tools.create_asset(name, DEST, cls, factory())
    )


def mesh(name):
    result = assets.load_asset(DEST + "/" + name + "/StaticMeshes/" + name)
    if not result:
        raise RuntimeError(name + " not imported")
    return result


fir = mesh("SM_WesternFir")
pine = mesh("SM_ShorePine")
fern = mesh("SM_SwordFern")
log = mesh("SM_NurseLog")
# The crowns may meet across the trail. Player collision follows only the sturdy stem.
for canopy in (fir, pine):
    body = canopy.get_editor_property("body_setup")
    capsule = u.KSphylElem()
    capsule.set_editor_properties({"center": u.Vector(0, 0, 468), "length": 900.0, "radius": 25.0})
    shape = u.KAggregateGeom()
    shape.set_editor_property("sphyl_elems", [capsule])
    body.set_editor_property("agg_geom", shape)
    assets.save_loaded_asset(canopy, False)

for label, a in by.items():
    if label.startswith(
        (
            "M6 forest ",
            "M5 tree ",
            "M5 branch ",
            "M4 organic ",
            "M4 branch ",
            "Placeholder tree",
            "Cascades nursery ",
        )
    ):
        actors.destroy_actor(a)
    elif label == "Boundary far":
        a.set_actor_enable_collision(False)
        a.set_actor_hidden_in_game(True)
    elif label.startswith(("M6 path ", "M6 cabin spur")):
        a.static_mesh_component.set_material(0, tread)
    elif label.startswith(
        ("M5 forest floor ", "M6 far forest floor", "Forest floor ", "M5 underforest")
    ):
        a.static_mesh_component.set_material(0, litter)
    elif label.startswith("M6 stone "):
        a.static_mesh_component.set_material(0, assets.load_asset(PRESENT + "/M_CabinStone"))

# A pair of continuous, textured side slopes give the forest a rising horizon.
# The corridor follows the actual spline and widens around the existing cabin yard.
stations = [
    spline.get_location_at_distance_along_spline(min(d, length), space)
    for d in range(0, math.ceil(length) + 151, 150)
]
station_x = [p.x for p in stations]


def centerline(x):
    idx = min(max(bisect.bisect_right(station_x, x) - 1, 0), len(stations) - 2)
    a, b = stations[idx : idx + 2]
    dx = max(0.01, b.x - a.x)
    t = min(max((x - a.x) / dx, 0), 1)
    return a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, math.hypot(1, (b.y - a.y) / dx)


def start_offset(x, side):
    near = max(0.0, 1 - max(0.0, abs(x - 2400.0) - 1000.0) / 1050.0) if side == 1 else 0.0
    return 370 + 1950 * near


def ridge(x, offset, side, trail_z):
    ramp = min(1, max(0, (offset - start_offset(x, side)) / 2400))
    roll = 95 * math.sin(x * 0.00084 + side * 0.9) + 75 * math.sin(x * 0.00197 + offset * 0.0017)
    return (
        trail_z
        - 21
        + ramp * (570 + roll + 90 * math.sin(offset * 0.0023 + x * 0.0009))
        + max(0, offset - 3000) * 0.06
    )


def terrain():
    verts = []
    uv = []
    tri = []
    for sign in (-1, 1):
        base = len(verts)
        for i, x in enumerate(range(-14500, 8651, 180)):
            y, z, normal_factor = centerline(x)
            start = start_offset(x, sign)
            offsets = [
                start + (7600 - start) * t
                for t in (0, 0.009, 0.032, 0.07, 0.12, 0.18, 0.27, 0.39, 0.54, 0.73, 1)
            ]
            for j, off in enumerate(offsets):
                v = u.Vector(x, y + sign * off * normal_factor, ridge(x, off, sign, z))
                verts.append(v)
                uv.append(u.Vector2D(x * 0.004, v.y * 0.004))
                if j and i:
                    d = len(verts) - 1
                    c = d - 1
                    b = d - len(offsets)
                    a = b - 1
                    for face in [(a, b, d), (a, d, c)]:
                        tri.append(u.IntVector(*(face if sign == 1 else face[::-1])))
    buf = u.GeometryScriptSimpleMeshBuffers()
    buf.set_editor_properties({"vertices": verts, "uv0": uv, "triangles": tri})
    shape = u.DynamicMesh()
    shape.append_buffers_to_mesh(buf)
    path = DEST + "/SM_TrailBanks"
    if not assets.does_asset_exist(path):
        opts = u.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_properties(
            {
                "enable_recompute_normals": True,
                "enable_recompute_tangents": True,
                "enable_collision": True,
                "collision_mode": u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
            }
        )
        bank, _ = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(
            shape, path, opts
        )
        bank.set_material(0, litter)
        assets.save_loaded_asset(bank)
    else:
        bank = assets.load_asset(path)
    a = by.get("Cascades wooded banks") or actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
    a.set_actor_label("Cascades wooded banks")
    a.static_mesh_component.set_static_mesh(bank)
    a.static_mesh_component.set_material(0, litter)


terrain()


def type_asset(name, geometry, cull, props):
    asset = get(name, u.FoliageType_InstancedStaticMesh, u.FoliageType_InstancedStaticMeshFactory)
    asset.set_editor_properties({"mesh": geometry, "cull_distance": u.Int32Interval(*cull), **props})
    assets.save_loaded_asset(asset, False)
    return asset


def canopy_type(name, geometry, scale, density, seed):
    f = type_asset(
        name,
        geometry,
        (13000, 18500),
        {
            "initial_seed_density": density,
            "collision_radius": 150.0,
            "shade_radius": 180.0,
            "num_steps": 2,
            "seeds_per_step": 2,
            "average_spread_distance": 430.0,
            "spread_variance": 180.0,
            "distribution_seed": seed,
            "procedural_scale": u.FloatInterval(*scale),
            "align_to_normal": False,
            "ground_slope_angle": u.FloatInterval(0.0, 38.0),
        },
    )
    body = f.get_editor_property("body_instance")
    body.set_editor_property("collision_profile_name", "BlockAll")
    f.set_editor_property("body_instance", body)
    assets.save_loaded_asset(f, False)
    return f


large_fir = canopy_type("FT_CoastalFir", fir, (0.75, 1.35), 1.52, 891)
large_pine = canopy_type("FT_ShorePine", pine, (0.80, 1.21), 0.87, 211)
saplings = type_asset(
    "FT_YoungFir",
    fir,
    (5500, 9200),
    {
        "initial_seed_density": 1.35,
        "collision_radius": 65.0,
        "shade_radius": 15.0,
        "num_steps": 2,
        "seeds_per_step": 3,
        "average_spread_distance": 230.0,
        "spread_variance": 90.0,
        "can_grow_in_shade": True,
        "distribution_seed": 451,
        "procedural_scale": u.FloatInterval(0.14, 0.43),
        "align_to_normal": False,
        "ground_slope_angle": u.FloatInterval(0.0, 42.0),
    },
)
fronds = type_asset(
    "FT_FernBed",
    fern,
    (3300, 6000),
    {
        "initial_seed_density": 2.3,
        "collision_radius": 28.0,
        "shade_radius": 4.0,
        "num_steps": 2,
        "seeds_per_step": 3,
        "average_spread_distance": 125.0,
        "spread_variance": 65.0,
        "can_grow_in_shade": True,
        "distribution_seed": 321,
        "procedural_scale": u.FloatInterval(0.65, 1.55),
        "cast_shadow": False,
    },
)
deadfall = type_asset("FT_NurseLog", log, (5000, 8500), {"cast_shadow": True})
verge = type_asset("FT_TrailFern", fern, (3300, 6000), {"cast_shadow": False})

fa = u.InstancedFoliageActor
for f in (large_fir, large_pine, saplings, fronds, deadfall, verge):
    fa.remove_all_instances(world, f)
spawner = get("PFS_CoastalStand", u.ProceduralFoliageSpawner, u.ProceduralFoliageSpawnerFactory)
types = []
for f in (large_fir, large_pine, saplings, fronds):
    w = u.FoliageTypeObject()
    w.set_editor_property("foliage_type_object", f)
    types.append(w)
spawner.set_editor_properties(
    {"foliage_types": types, "random_seed": 71, "tile_size": 3800, "num_unique_tiles": 3}
)
assets.save_loaded_asset(spawner, False)
options = u.GeometryScriptCreateNewVolumeFromMeshOptions()
options.set_editor_property("volume_type", u.ProceduralFoliageVolume)
dynamic = u.DynamicMesh()
dynamic.append_box(u.GeometryScriptPrimitiveOptions(), u.Transform(), 1550, 1380, 2000)
vols = []
for d in range(800, int(length) - 300, 1490):
    p = spline.get_location_at_distance_along_spline(d, space)
    v = spline.get_direction_at_distance_along_spline(d, space)
    yaw = math.degrees(math.atan2(v.y, v.x))
    for sign in (-1, 1):
        if sign == 1 and 200 < p.x < 4300:
            continue
        ofs = 1240 + 90 * math.sin(d * 0.00103 + sign * 3)
        center = (p.x - sign * ofs * v.y, p.y + sign * ofs * v.x, p.z - 500)
        xform = u.Transform(location=center, rotation=u.Rotator(yaw=yaw))
        actor, result = u.GeometryScript_NewAssetUtils.create_new_volume_from_mesh_world_context(
            world, dynamic, xform, "Forest Nursery", options
        )
        if not actor:
            raise RuntimeError("Missing foliage volume")
        actor.set_actor_label("Cascades nursery %05d %d" % (d, sign))
        actor.get_editor_property("procedural_component").set_editor_property("foliage_spawner", spawner)
        vols.append(actor)
u.ProceduralFoliageEditorLibrary.resimulate_procedural_foliage_volumes(vols)


def instance_counts():
    counts = {}
    for actor in actors.get_all_level_actors():
        if isinstance(actor, fa):
            for component in actor.get_components_by_class(u.InstancedStaticMeshComponent):
                if component.static_mesh:
                    key = component.static_mesh.get_name()
                    counts[key] = counts.get(key, 0) + component.get_instance_count()
    return counts


counts = instance_counts()
u.log("CASCADES_PFT " + str(len(vols)) + " " + str(counts))
if sum(counts.values()) < 100:
    raise RuntimeError("Simulation requires the full editor physics scene")

# Swaths of ferns hug the shoulders. Hand-shaped pocket sizes avoid two parallel hedges.
fern_trans = []
wood_trans = []
for d in range(50, int(length) - 180, 65):
    p = spline.get_location_at_distance_along_spline(d, space)
    v = spline.get_direction_at_distance_along_spline(d, space)
    for sign in (-1, 1):
        if sign == 1 and 500 < p.x < 4100:
            continue
        if rand.random() > 0.54 + 0.42 * math.sin(d * 0.0024 + sign * 7):
            continue
        for _ in range(rand.randrange(1, 5)):
            off = rand.uniform(196, 435)
            fwd = rand.uniform(-110, 110)
            x = p.x - sign * v.y * off + v.x * fwd
            y = p.y + sign * v.x * off + v.y * fwd
            cy, cz, n = centerline(x)
            z = cz - (6 if off < 370 else 11)
            s = rand.uniform(0.58, 1.44)
            fern_trans.append(
                u.Transform(
                    location=(x, y, z), rotation=u.Rotator(yaw=rand.uniform(0, 360)), scale=(s, s, s)
                )
            )
        if rand.random() < 0.082:
            off = rand.uniform(470, 1050)
            x = p.x - sign * v.y * off
            y = p.y + sign * v.x * off
            cy, cz, n = centerline(x)
            z = max(cz - 10, ridge(x, abs(cy - y) / n, sign, cz))
            s = rand.uniform(0.60, 1.31)
            wood_trans.append(
                u.Transform(
                    location=(x, y, z),
                    rotation=u.Rotator(yaw=math.degrees(math.atan2(v.y, v.x)) + rand.uniform(-40, 40)),
                    scale=(s, s, s),
                )
            )
fa.add_instances_and_wait(world, verge, fern_trans)
fa.add_instances_and_wait(world, deadfall, wood_trans)

# The distant, overlapping evergreen canopy comes from a separate runtime PCG graph.
graph = get("PCG_RidgeCanopy", u.PCGGraph, u.PCGGraphFactory)
if not graph.get_editor_property("nodes"):
    grid, gs = graph.add_node_of_type(u.PCGCreatePointsGridSettings)
    gs.set_editor_properties(
        {
            "grid_extents": u.Vector(550, 500, 0),
            "cell_size": u.Vector(550, 500, 100),
            "coordinate_space": u.PCGCoordinateSpace.ORIGINAL_COMPONENT,
        }
    )
    jitter, js = graph.add_node_of_type(u.PCGTransformPointsSettings)
    js.set_editor_properties(
        {
            "offset_min": u.Vector(-140, -130, 0),
            "offset_max": u.Vector(140, 130, 0),
            "rotation_min": u.Rotator(yaw=-175),
            "rotation_max": u.Rotator(yaw=175),
            "scale_min": u.Vector(0.91, 0.91, 0.91),
            "scale_max": u.Vector(1.65, 1.65, 1.65),
            "uniform_scale": True,
        }
    )
    hit, hs = graph.add_node_of_type(u.PCGWorldRayHitSettings)
    project, ps = graph.add_node_of_type(u.PCGProjectionSettings)
    pr = ps.get_editor_property("projection_params")
    pr.set_editor_property("project_rotations", False)
    ps.set_editor_property("projection_params", pr)
    spawn, ss = graph.add_node_of_type(u.PCGStaticMeshSpawnerSettings)
    entries = []
    for model, weight in [(fir, 5), (pine, 3)]:
        entry = u.PCGMeshSelectorWeightedEntry()
        desc = entry.get_editor_property("descriptor")
        desc.set_editor_properties(
            {
                "static_mesh": model,
                "instance_start_cull_distance": 16000,
                "instance_end_cull_distance": 23500,
            }
        )
        entry.set_editor_properties({"descriptor": desc, "weight": weight})
        entries.append(entry)
    ss.get_editor_property("mesh_selector_parameters").set_editor_property("mesh_entries", entries)
    graph.add_edge(grid, "Out", jitter, "In")
    graph.add_edge(jitter, "Out", project, "In")
    graph.add_edge(hit, "Out", project, "Projection Target")
    graph.add_edge(project, "Out", spawn, "In")
    graph.add_edge(spawn, "Out", graph.get_output_node(), "Out")
    assets.save_loaded_asset(graph)
for label, actor in by.items():
    if label.startswith("PCG forest patch "):
        pcg = actor.get_editor_property("pcg_component")
        pcg.set_graph(graph)
        p = actor.get_actor_location()
        _, trail_z, _ = centerline(p.x)
        actor.set_actor_location(u.Vector(p.x, p.y, trail_z + 300), False, False)

# Cool fill preserves bark and fern detail in moon shadows.
fill = by.get("Cascades diffuse sky fill") or actors.spawn_actor_from_class(
    u.DirectionalLight, u.Vector(0, 0, 730)
)
fill.set_actor_label("Cascades diffuse sky fill")
fill.set_actor_rotation(u.Rotator(pitch=-64, yaw=122), False)
fill.light_component.set_editor_property("cast_shadows", False)
fill.light_component.set_intensity(0.22)
fill.light_component.set_light_color(u.LinearColor(0.35, 0.52, 0.82, 1))
fog = by["M4 moonlit mist"].component
fog.set_fog_density(0.032)
fog.set_fog_height_falloff(0.16)
fog.set_fog_inscattering_color(u.LinearColor(0.026, 0.043, 0.069, 1))
fog.set_fog_max_opacity(0.91)
fog.set_volumetric_fog(True)
fog.set_volumetric_fog_distance(7200.0)
if not levels.save_current_level():
    raise RuntimeError("Saving map failed")
u.log(
    "CASCADES_SAVED "
    + str(instance_counts())
    + " verge "
    + str(len(fern_trans))
    + " logs "
    + str(len(wood_trans))
)
