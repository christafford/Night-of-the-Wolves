"""Stitch the through-trail and cottage turnoff into one walkable clearing in M1_Trail."""

import bisect
import sys
from pathlib import Path

import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from woodland_surfaces import DEST, assets, build

MAP = "/Game/NightOfTheWolves/Maps/M1_Trail"
MESH = DEST + "/SM_CottageClearing"
LABEL = "Cottage clearing"
# The interior edge sits within the jambs, which span X=2400..2500, Y outside +/-90.
FRONT = 2260
SILL = 2450
HALF_WIDTH = 160
BOTTOM = -250
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)


def clearing_mesh(by):
    spline = by["M5 pursuit route"].spline
    world = u.SplineCoordinateSpace.WORLD
    stations = [
        spline.get_location_at_spline_point(i, world)
        for i in range(spline.get_number_of_spline_points())
    ]
    station_x = [p.x for p in stations]
    if not all(a < b for a, b in zip(station_x, station_x[1:])):
        raise RuntimeError("Expected the trail stations to run east through the clearing")

    def center(x):
        i = bisect.bisect_right(station_x, x) - 1
        a, b = stations[i:i + 2]
        t = (x - a.x) / (b.x - a.x)
        return a.y + t * (b.y - a.y), a.z + t * (b.z - a.z)

    vertices, triangles, uv = [], [], []

    def vertex(p, across):
        vertices.append(p)
        uv.append(u.Vector2D(p.x / 100, across / 7.5))
        return len(vertices) - 1

    def quad(a, b, c, d):
        triangles.extend([u.IntVector(a, b, c), u.IntVector(a, c, d)])

    # Break at the actual trap rim. No triangle spans the removable cover.
    interior_stations = {x for x in station_x if -500 < x < 4500}
    xs = sorted(interior_stations | {-500, 4500, FRONT, SILL})
    rows = []
    for x in xs:
        y, z = center(x)
        profile = [(-2200, 0), (y - 650, 0), (y - 375, z), (y, z),
                   (y + 375, z), (-300, 0), (-HALF_WIDTH, 0), (HALF_WIDTH, 0),
                   (300, 0), (2200, 0)]
        row = [vertex(u.Vector(x, py, pz), py - y) for py, pz in profile]
        if rows:
            old = rows[-1]
            for j in range(len(row) - 1):
                if FRONT < x <= SILL and profile[j][0] == -HALF_WIDTH:
                    continue
                quad(old[j], old[j + 1], row[j + 1], row[j])
        rows.append(row)

    # At each end, share the top edge of the retained, pitched trail block precisely.
    # Slanted edge coordinates matter on the eastern bend.
    for start, index, row in [(True, 55, rows[0]), (False, 75, rows[-1])]:
        tf = by["M6 path %03d" % index].get_actor_transform()
        end_x = 50 if start else -50
        edge = [vertex(tf.transform_location(u.Vector(end_x, across, 50)), across * 7.5)
                for across in (-50, 0, 50)]
        left, right = (edge, row[2:5]) if start else (row[2:5], edge)
        for j in (0, 1):
            quad(left[j], left[j + 1], right[j + 1], right[j])

    # Give the dug trap inward-facing earth walls down to the existing pit bottom.
    # Keep all intermediate spline stations in the rim to avoid a T-junction.
    rim_x = [x for x in xs if FRONT <= x <= SILL]
    rim = [(x, -HALF_WIDTH) for x in rim_x] + [(x, HALF_WIDTH) for x in reversed(rim_x)]
    for (x1, y1), (x2, y2) in zip(rim, rim[1:] + rim[:1]):
        quad(vertex(u.Vector(x1, y1, 0), 600), vertex(u.Vector(x1, y1, BOTTOM), 600),
             vertex(u.Vector(x2, y2, BOTTOM), 600), vertex(u.Vector(x2, y2, 0), 600))

    buffers = u.GeometryScriptSimpleMeshBuffers()
    buffers.set_editor_properties({"vertices": vertices, "uv0": uv, "triangles": triangles})
    shape = u.DynamicMesh()
    shape.append_buffers_to_mesh(buffers)
    if assets.does_asset_exist(MESH):
        target = assets.load_asset(MESH)
        opts = u.GeometryScriptCopyMeshToAssetOptions()
        opts.set_editor_properties({"enable_recompute_normals": True, "enable_recompute_tangents": True})
        _, result = u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
            shape, target, opts, u.GeometryScriptMeshWriteLOD())
    else:
        opts = u.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_properties({
            "enable_recompute_normals": True, "enable_recompute_tangents": True,
            "enable_collision": True, "collision_mode": u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
        })
        target, result = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(
            shape, MESH, opts)
    if result != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Could not write " + MESH)
    return target


def doorway_headroom(by):
    # Raise the opening under the existing pitched gable. Separate the shallow
    # ceiling recess from the surrounding 300 cm ceiling with edge-to-edge slabs.
    original_roof = by["Cabin roof"].static_mesh_component
    cube = original_roof.static_mesh
    stone = original_roof.get_material(0)

    def block(label, x0, x1, y0, y1, z0, z1):
        actor = by.get(label)
        if actor is None:
            actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
            actor.set_actor_label(label)
            actor.static_mesh_component.set_static_mesh(cube)
            actor.static_mesh_component.set_material(0, stone)
            actor.static_mesh_component.set_collision_profile_name("BlockAll")
        actor.modify()
        center = u.Vector((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
        actor.set_actor_location(center, False, False)
        actor.set_actor_scale3d(u.Vector((x1 - x0) / 100, (y1 - y0) / 100, (z1 - z0) / 100))

    block("Cabin door", 2435, 2465, -90, 90, 0, 324)
    block("Cabin lintel", 2400, 2500, -90, 90, 324, 350)
    block("Cabin front left header", 2400, 2500, -410, -90, 300, 350)
    block("Cabin front right header", 2400, 2500, 90, 410, 300, 350)
    block("Cabin entrance ceiling", 2500, 2680, -100, 100, 324, 350)
    block("Cabin ceiling left", 2500, 2680, -410, -100, 300, 350)
    block("Cabin ceiling right", 2500, 2680, 100, 410, 300, 350)
    block("Cabin roof", 2680, 3260, -410, 410, 300, 350)


def install():
    if not levels.load_level(MAP):
        raise RuntimeError("Could not open " + MAP)
    by = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
    obsolete = ["Forest floor left", "Forest floor right", "Trail", "Trail beyond pit",
                "Pit side left", "Pit side right", "M6 cabin spur"]
    obsolete += ["M6 path %03d" % i for i in range(56, 75)]
    if LABEL not in by and not all(label in by for label in obsolete):
        raise RuntimeError("Cottage blockout differs from the expected yard")
    material = build("M_CottageGround", True, cottage=True)
    target = clearing_mesh(by)
    target.set_material(0, material)
    if not assets.save_loaded_asset(target, False):
        raise RuntimeError("Could not save the stitched ground")

    clearing = by.get(LABEL) or actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
    clearing.set_actor_label(LABEL)
    clearing.static_mesh_component.set_static_mesh(target)
    clearing.static_mesh_component.set_collision_profile_name("BlockAll")
    # Keep the tagged cover and the bottom exactly within the new cutout.
    for label in ("Pit cover", "Pit bottom"):
        actor = by[label]
        actor.modify()
        actor.set_actor_location(u.Vector((FRONT + SILL) / 2, 0, actor.get_actor_location().z),
                                 False, False)
        actor.set_actor_scale3d(u.Vector((SILL - FRONT) / 100, HALF_WIDTH * 2 / 100,
                                         actor.get_actor_scale3d().z))
    by["Pit cover"].static_mesh_component.set_material(0, assets.load_asset(DEST + "/M_NeedleLitter"))
    doorway_headroom(by)
    for label in obsolete:
        if label in by and not actors.destroy_actor(by[label]):
            raise RuntimeError("Could not remove overlapping surface " + label)
    if not levels.save_current_level():
        raise RuntimeError("Could not save " + MAP)
    u.log("COTTAGE CLEARING SAVED " + MESH)


if __name__ == "__main__":
    install()
