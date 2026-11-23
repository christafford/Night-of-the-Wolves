"""Fit a button-front service tunic, crossed bandoliers, field trousers and riding boots."""

from collections import defaultdict
from functools import lru_cache
from math import atan2, cos, pi, sin
from pathlib import Path
import sys

import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import create_max_outfit as sew

ROOT = "/Game/NightOfTheWolves/Bandit"
FABRIC = "/Game/NightOfTheWolves/Max/Wardrobe/M_TrailFabric"
SWATCHES = {
    "FieldKhaki": ((0.207, 0.148, 0.079), 0.90, 0.48),
    "TrouserWool": ((0.165, 0.146, 0.106), 0.92, 1.18),
    "HarnessLeather": ((0.147, 0.073, 0.033), 0.59, 0.13),
    "RidingLeather": ((0.074, 0.044, 0.031), 0.64, 0.15),
    "BlackedSoles": ((0.030, 0.023, 0.018), 0.78, 0.12),
    "TarnishedBrass": ((0.57, 0.355, 0.143), 0.39, 0.09),
    "CopperPatina": ((0.31, 0.127, 0.061), 0.48, 0.09),
}
TAU = 2 * pi
assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary


def materials():
    tools = u.AssetToolsHelpers.get_asset_tools()
    cloth = assets.load_asset(FABRIC)
    name = "M_BanditMetal"
    metal = assets.load_asset(ROOT + "/" + name) if assets.does_asset_exist(ROOT + "/" + name) else None
    if not metal:
        metal = tools.duplicate_asset(name, ROOT, cloth)
        n = edit.create_material_expression(metal, u.MaterialExpressionConstant, -180, 750)
        n.set_editor_property("r", 0.63)
        edit.connect_material_property(n, "", u.MaterialProperty.MP_METALLIC)
        assets.save_loaded_asset(metal, False)
    name = "M_BanditTwill"
    path = ROOT + "/" + name
    twill = assets.load_asset(path) if assets.does_asset_exist(path) else None
    if not twill:
        twill = tools.create_asset(name, ROOT, u.Material, u.MaterialFactoryNew())
        twill.set_editor_properties({"two_sided": True, "used_with_skeletal_mesh": True})
        tint = edit.create_material_expression(twill, u.MaterialExpressionVectorParameter)
        tint.set_editor_property("parameter_name", "Tint")
        tint.set_editor_property("default_value", u.LinearColor(*SWATCHES["FieldKhaki"][0], 1))
        edit.connect_material_property(tint, "", u.MaterialProperty.MP_BASE_COLOR)
        for value, slot in (
            (0.91, u.MaterialProperty.MP_ROUGHNESS),
            (0.18, u.MaterialProperty.MP_SPECULAR),
        ):
            constant = edit.create_material_expression(twill, u.MaterialExpressionConstant)
            constant.set_editor_property("r", value)
            edit.connect_material_property(constant, "", slot)
        assets.save_loaded_asset(twill, False)
    slots = {}
    for key, (rgb, roughness, weave) in SWATCHES.items():
        name = "MI_" + key
        path = ROOT + "/" + name
        mi = (
            assets.load_asset(path)
            if assets.does_asset_exist(path)
            else tools.create_asset(
                name, ROOT, u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew()
            )
        )
        edit.set_material_instance_parent(
            mi,
            metal
            if key in ("TarnishedBrass", "CopperPatina")
            else (twill if key == "FieldKhaki" else cloth),
        )
        edit.set_material_instance_vector_parameter_value(mi, "Tint", u.LinearColor(*rgb, 1))
        edit.set_material_instance_scalar_parameter_value(mi, "Roughness", roughness)
        edit.set_material_instance_scalar_parameter_value(mi, "Weave", weave)
        if not assets.save_loaded_asset(mi, False):
            raise RuntimeError("Could not save " + path)
        slots[key] = mi
    return slots


class CampaignPanels(sew.Panels):
    def __init__(self):
        self.by_mat = {key: [[], [], [], {}] for key in SWATCHES}

    def mesh_sections(self):
        # Keep section indices stable when sewing separately weighted layers.
        target = u.DynamicMesh()
        for index, (vertices, uv, triangles, _) in enumerate(self.by_mat.values()):
            if not triangles:
                continue
            buffers = u.GeometryScriptSimpleMeshBuffers()
            buffers.set_editor_properties(
                {
                    "vertices": [u.Vector(*v) for v in vertices],
                    "triangles": [u.IntVector(*t) for t in triangles],
                    "uv0": [u.Vector2D(*v) for v in uv],
                }
            )
            target.append_buffers_to_mesh(buffers, index)
        target.set_per_vertex_normals()
        return target

    def patched_sleeve(self, at, overlay):

        # Fold a tailored cuff around the wrist, including the inside edge seen
        # when he raises his hands. Both surfaces follow the same sleeve ring.
        def ring(t):
            points = [at(t, j / 12) for j in range(12)]
            return tuple(sum(v[k] for v in points) / len(points) for k in range(3))

        wrist = ring(1.0)
        forearm = ring(0.90)
        along = sew.unit(sew.sub(wrist, forearm))

        def folded(t, j):
            location = at(t, j)
            middle = sew.lerp(forearm, wrist, (t - 0.90) / 0.10)
            return sew.lift(location, sew.unit(sew.sub(location, middle)), 0.16)

        self.grid(10, 48, lambda t, j: folded(0.915 + 0.085 * t, j), "FieldKhaki", True)

        def lining(t, j):
            outer = folded(1.0, j)
            return sew.lift(sew.lerp(outer, wrist, 0.24 * t), along, -0.34 * t)

        self.grid(3, 48, lining, "FieldKhaki", True)
        self.cord([folded(0.921, j / 48) for j in range(49)], 0.07, "FieldKhaki")

    def cord(self, path, r, mat):
        if mat in ("OchreYoke", "Laces"):
            return
        super().cord(
            path,
            r,
            {
                "IndigoShell": "FieldKhaki",
                "Rubber": "BlackedSoles",
                "OxbloodCanvas": "RidingLeather",
                "DarkBinding": "RidingLeather",
            }.get(mat, mat),
        )

    def tri(self, mat, points, uv):
        super().tri(
            {
                "WashedDenim": "TrouserWool",
                "OxbloodCanvas": "RidingLeather",
                "Rubber": "BlackedSoles",
            }.get(mat, mat),
            points,
            uv,
        )


def lathe(panels, base, axis, sections, sides=10):
    """Turn the button, rivet or cartridge silhouette around its axis."""
    direction = sew.unit(axis)
    right = sew.unit(sew.cross(direction, (0, 1, 0) if abs(direction[1]) < 0.85 else (1, 0, 0)))
    out = sew.cross(direction, right)
    for (z, r, material), (z2, r2, _) in zip(sections, sections[1:]):

        def vertex(t, j):
            mid = sew.lift(base, direction, sew.mix(z, z2, t))
            normal = sew.add(sew.scale(right, cos(TAU * j)), sew.scale(out, sin(TAU * j)))
            return sew.lift(mid, normal, sew.mix(r, r2, t))

        panels.grid(2, sides, vertex, material)


def make_coat(tailor, garment):
    p, s = tailor.panels, tailor.s
    hem = tailor.hip - 10 * s

    def shell(h, a):
        if h < 0.67:
            z = sew.mix(hem - 1.45 * s * cos(2 * a), tailor.chest, h / 0.67)
            ease = s * (2.8 + 0.55 * cos(6 * a + 7 * h) + 0.6 * (1 - h))
            return tailor.torso(z, a, ease)
        ridge = (
            sin(a) * (abs(tailor.shoulder[0]) + 3.3 * s),
            tailor.neck[1] + cos(a) * (9.6 + max(0, cos(a))) * s,
            tailor.shoulder[2] + (3.75 - 4.2 * max(0, cos(a)) ** 2 - 0.55 * abs(cos(a))) * s,
        )
        if h < 0.865:
            return sew.lerp(shell(0.6699, a), ridge, (h - 0.67) / 0.195)
        neck = (
            6.15 * s * sin(a),
            tailor.neck[1] + 6.25 * s * cos(a),
            tailor.neck[2] + s * (-0.9 - 4.9 * max(0, cos(a)) ** 2),
        )
        return sew.lerp(ridge, neck, (h - 0.865) / 0.135)

    # A separate flared skirt is sewn below the waist. Above this the shirt
    # is continuous across the neckline, shoulder and lifted arm.
    hem_panels = CampaignPanels()
    hem_panels.grid(19, 82, lambda t, j: shell(t * 0.35, TAU * j), "FieldKhaki")
    skirt = hem_panels.mesh_sections()
    tailor.body.transfer_bone_weights_from_mesh(skirt)
    garment.append_mesh(skirt, u.Transform())
    bvh = garment.build_bvh_for_mesh()[-1]

    @lru_cache(maxsize=50000)
    def surface(h, a):
        approx = shell(h, a)
        rise = 2.6 * sew.clamp((h - 0.69) / 0.27, 0, 1) ** 2
        direction = sew.unit((sin(a), cos(a), rise))
        origin = sew.lift(approx, direction, 20 * s)
        ray = sew.scale(direction, -1)
        hit = garment.find_nearest_ray_intersection_with_mesh(
            bvh, u.Vector(*origin), u.Vector(*ray), tailor.query
        )[-2]
        if hit.hit and abs(hit.ray_parameter - 20 * s) < 10 * s:
            return sew.lift(origin, ray, hit.ray_parameter), direction
        return approx, direction

    def cover(h, a, offset):
        pos, direction = surface(h, a)
        return sew.lift(pos, direction, offset * s)

    # The center placket and its brass fastening row remain visible below the crossed straps.
    p.grid(25, 3, lambda t, j: cover(0.02 + t * 0.97, (j - 0.5) * 0.113, 0.21), "FieldKhaki")
    for h in (0.115, 0.277, 0.443, 0.610, 0.75, 0.958):
        c = cover(h, 0, 0.42)
        lathe(
            p,
            c,
            (0, 1, 0),
            [
                (0, 0.03, "TarnishedBrass"),
                (0.02, 0.66 * s, "TarnishedBrass"),
                (0.22 * s, 0.56 * s, "TarnishedBrass"),
                (0.30 * s, 0.015, "TarnishedBrass"),
            ],
            12,
        )

    for side in (-1, 1):

        def pocket(t, j):
            return cover(
                0.635 + 0.158 * t + 0.015 * abs(j - 0.5) * 2,
                side * (0.43 + j * 0.59),
                0.38 + 0.32 * sin(pi * j) * sin(pi * t),
            )

        def flap(t, j):
            return cover(
                0.78 + 0.037 * t - 0.041 * (1 - abs(2 * j - 1)) * (1 - t), side * (0.42 + j * 0.60), 0.82
            )

        p.grid(9, 14, pocket, "FieldKhaki", side < 0)
        p.grid(5, 14, flap, "FieldKhaki", side < 0)
        p.cord([flap(0, j / 20) for j in range(21)], 0.075 * s, "FieldKhaki")
    p.cord([cover(0.014, TAU * j / 90, 0.18) for j in range(91)], 0.13 * s, "FieldKhaki")
    return cover


def bandolier(tailor, cover, side):
    p, s = tailor.panels, tailor.s

    def center(t, back):
        # Cross diagonally from the neck side of a shoulder to the opposite hip.
        h = sew.mix(0.887, 0.285, t)
        a = side * sew.mix(0.83, -1.45, t)
        return h, pi - a if back else a

    def coords(t, j, back=False):
        h, a = center(t, back)
        dh = sew.scale(sew.sub(cover(h + 0.001, a, 0), cover(h - 0.001, a, 0)), 500.0)
        da = sew.scale(sew.sub(cover(h, a + 0.001, 0), cover(h, a - 0.001, 0)), 500.0)
        normal = sew.unit(sew.cross(dh, da))
        axis = sew.unit(sew.add(sew.scale(dh, -0.602), sew.scale(da, (1 if back else -1) * 2.28 * side)))
        crosswise = sew.unit(sew.cross(axis, normal))
        d = (j - 0.5) * 6.05 * s
        hh, aa, ha = sew.dot(dh, dh), sew.dot(da, da), sew.dot(dh, da)
        det = max(0.001, hh * aa - ha * ha)
        x, y = sew.dot(dh, crosswise), sew.dot(da, crosswise)
        # A graduated offset lets the upper harness pass over the lower cartridges at the X.
        over = 2.35 * sew.bell(t, 0.39, 0.22) if side == 1 and not back else 0.15
        return (
            h + d * (x * aa - y * ha) / det,
            a + d * (y * hh - x * ha) / det,
            1.43 + over - 1.04 * max(sew.bell(t, 0, 0.17), sew.bell(t, 1, 0.18)),
        )

    def point(t, j, back=False, extra=0):
        h, a, d = coords(t, j, back)
        return cover(h, a, d + extra)

    def join(end, t, j):
        h0, a0, d0 = coords(end, j)
        h1, a1, d1 = coords(end, 1 - j, True)
        # Follow the coat around the shoulder and hip instead of cutting a
        # straight chord through the sleeve and leaving only exposed tips.
        turn = (a1 - a0 + pi) % TAU - pi
        return cover(sew.mix(h0, h1, t), a0 + t * turn, sew.mix(d0, d1, t))

    for back in (False, True):
        p.grid(41, 6, lambda t, j: point(t, j, back), "HarnessLeather", True)
        for j in (0.03, 0.97):
            p.cord([point(t / 42, j, back, 0.065) for t in range(43)], 0.11 * s, "RidingLeather")
        for end in (0.0, 1.0):
            if back:
                p.grid(
                    9,
                    5,
                    lambda t, j: join(end, t, j),
                    "HarnessLeather",
                )

    if side == 1:
        # The doubled leather tongue and squared brass buckle sit at the collarbone.
        p.grid(
            12, 5, lambda t, j: point(0.055 + 0.143 * t, 0.10 + 0.80 * j, False, 0.19), "RidingLeather"
        )
        rim = [
            (0.085, 0.09),
            (0.078, 0.13),
            (0.078, 0.87),
            (0.085, 0.91),
            (0.154, 0.91),
            (0.162, 0.87),
            (0.162, 0.13),
            (0.154, 0.09),
            (0.085, 0.09),
        ]
        p.cord([point(t, j, False, 0.46) for t, j in rim], 0.24 * s, "TarnishedBrass")
        p.cord([point(t, 0.5, False, 0.55) for t in (0.076, 0.10, 0.13)], 0.17 * s, "TarnishedBrass")
        for j in (0.22, 0.78):
            c = point(0.181, j, False, 0.42)
            n = sew.sub(point(0.181, j, False, 1.0), c)
            lathe(
                p,
                c,
                n,
                [
                    (0, 0.32 * s, "TarnishedBrass"),
                    (0.12 * s, 0.30 * s, "TarnishedBrass"),
                    (0.19 * s, 0, "TarnishedBrass"),
                ],
            )

    # Each round is a separate object in the baked wardrobe. Dark leather loops
    # interrupt the brass row, leaving its stepped ends legible at game distance.
    for i in range(21):
        t = 0.071 + 0.90 * i / 20
        if side == 1 and t < 0.21:
            continue  # The shoulder fastener takes the place of these loops.
        near = point(t, 0.5)
        normal = sew.unit(sew.sub(point(t, 0.5, False, 0.4), near))
        along = sew.unit(sew.sub(point(t, 0.96), point(t, 0.04)))
        mid = sew.lift(near, normal, 0.61 * s)
        bronze, tip = "TarnishedBrass", "CopperPatina"
        rings = [
            (-3.47, 0.05, bronze),
            (-3.44, 0.52, bronze),
            (-3.22, 0.52, bronze),
            (-3.13, 0.44, bronze),
            (-2.75, 0.49, bronze),
            (0.66, 0.445, bronze),
            (0.96, 0.33, bronze),
            (1.45, 0.31, tip),
            (2.05, 0.28, tip),
            (3.36, 0.035, tip),
        ]
        lathe(p, mid, along, [(z * s, r * s, m) for z, r, m in rings])
        # A loop rises over the case and returns to the belt on either side.
        tangent = sew.unit(sew.sub(point(t + 0.002, 0.50), near))

        def loop(tall, j):
            a = pi * j
            ridge = sew.add(sew.scale(tangent, 0.69 * s * cos(a)), sew.scale(normal, 0.69 * s * sin(a)))
            return sew.add(sew.lift(mid, along, sew.mix(-1.31, 0.40, tall) * s), ridge)

        p.grid(3, 10, loop, "HarnessLeather", True)


def continuous_tunic(tailor, positions, faces):
    # Fit the torso and sleeves in a single connected surface. Raise the woven
    # cloth off the skin and stop the sleeves just inside the stitched cuffs.
    p, s, neck = CampaignPanels(), tailor.s, tailor.neck
    selected = []
    normals = [[0.0, 0.0, 0.0] for _ in positions]
    arms = {}
    for side in ("l", "r"):
        elbow, hand = (tailor.b[bone + "_" + side] for bone in ("lowerarm", "hand"))
        arms[side] = (elbow, hand, sew.unit(sew.sub(hand, elbow)))

    def within_shirt(q):
        x, y, z = q
        if abs(x) > abs(tailor.shoulder[0]) + 5 * s:
            elbow, hand, axis = arms["l" if x > 0 else "r"]
            if sew.dot(sew.sub(q, hand), axis) > -0.4 * s:
                return False
        elif z < tailor.hip + 1.4 * s:
            return False
        front = sew.clamp((y - neck[1]) / (6 * s), 0, 1)
        return z <= neck[2] + (1.0 - 3.8 * front) * s

    for face in faces:
        pts = [positions[i] for i in face]
        if not all(within_shirt(q) for q in pts):
            continue
        n = sew.cross(sew.sub(pts[2], pts[0]), sew.sub(pts[1], pts[0]))
        for i in face:
            normals[i] = sew.add(normals[i], n)
        selected.append(face)

    fitted = {}
    for face in selected:
        for i in face:
            if i in fitted:
                continue
            x, y, z = q = positions[i]
            a = atan2(x, y - neck[1])
            height = (z - tailor.hip) / (neck[2] - tailor.hip)
            ease = 2.30 + 0.24 * cos(5 * a + 4 * height)
            arm = sew.clamp((abs(x) - abs(tailor.shoulder[0])) / (9 * s), 0, 1)
            elbow, hand, axis = arms["l" if x > 0 else "r"]
            forearm = sew.dot(sew.sub(q, elbow), axis) / max(1, sew.dot(sew.sub(hand, elbow), axis))
            cuff = sew.clamp((forearm - 0.78) / 0.20, 0, 1)
            ease = sew.mix(ease, 1.65 + 0.24 * cos(23 * forearm), arm)
            ease = sew.mix(ease, 0.52, cuff * arm)
            normal = sew.unit(normals[i])
            point = sew.lift(q, normal, s * ease)
            fitted[i] = point, ((a + pi) / TAU, height)
        p.tri("FieldKhaki", [fitted[i][0] for i in face], [fitted[i][1] for i in face])
    # The MetaHuman face originally supplied a shoulder bib. The body alone
    # terminates on an open loop reaching from deltoid to deltoid. Replace it
    # with a sewn yoke. Its outermost vertices exactly match that same loop.
    edge_faces = defaultdict(list)
    for a, b, c in selected:
        for i, j in ((a, b), (b, c), (c, a)):
            edge_faces[tuple(sorted((i, j)))].append((i, j))

    # Fit the short stand collar around the actual Creator throat. Since the
    # skull and shoulder bib form one mesh, this can be measured before the
    # face material removes the bib at runtime.
    face_mesh = u.DynamicMesh()
    face = assets.load_asset(ROOT + "/Villa_Head")
    _, read = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        face, face_mesh, u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD()
    )
    if read != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Author the bandit's Creator face before fitting the service collar")
    face_bvh = face_mesh.build_bvh_for_mesh()[-1]

    @lru_cache(maxsize=512)
    def collar_ring(a, h):
        forward = max(0, cos(a))
        z = neck[2] + (1.05 - 1.95 * forward + h * (3.45 - 0.5 * forward)) * s
        mid = (0, neck[1], z)
        n = sew.circular(a)
        hit = face_mesh.find_nearest_ray_intersection_with_mesh(
            face_bvh, u.Vector(*mid), u.Vector(*n), tailor.query
        )[-2]
        fallback = (7.65 + 1.15 * forward) * s
        skin = sew.clamp(hit.ray_parameter if hit.hit else fallback, 5.9 * s, 10.8 * s)
        return sew.lift(mid, n, skin + (0.83 - 0.14 * h) * s)

    def neckline(i, t):
        point, (u_coord, v_coord) = fitted[i]
        x, y, _ = point
        a = atan2(x, y - neck[1])
        if t > 1:
            return collar_ring(a, t - 1), (u_coord, 1.03 + 0.06 * (t - 1))
        collar = collar_ring(a, 0)
        sewn = sew.lerp(point, collar, t)
        sewn = sew.add(sewn, (0, 0, s * 0.75 * sin(pi * t)))
        return sewn, (u_coord, sew.mix(v_coord, 1.03, t))

    seam_weights = {}
    num_edges = 0
    for edges in edge_faces.values():
        if len(edges) != 1:
            continue
        i, j = edges[0]
        if any(
            positions[v][2] < tailor.chest - 1.5 * s
            or abs(positions[v][0]) > abs(tailor.shoulder[0]) + 3 * s
            for v in (i, j)
        ):
            continue
        num_edges += 1
        breaks = [n / 8.0 for n in range(9)] + [1.12, 1.55, 1.92, 2.0]
        for a, b in zip(breaks, breaks[1:]):
            v0, v1 = neckline(j, a), neckline(i, a)
            v2, v3 = neckline(i, b), neckline(j, b)
            for (point, _), source, progress in ((v0, j, a), (v1, i, a), (v2, i, b), (v3, j, b)):
                if progress > 0:
                    seam_weights[point] = source, min(progress, 1.0)
            for corners in ((v0, v1, v2), (v0, v2, v3)):
                p.tri("FieldKhaki", [v[0] for v in corners], [v[1] for v in corners])
    if num_edges < 20:
        raise RuntimeError(f"Could not find the upper-body seam ({num_edges} edges)")
    u.log(f"SEWN UPPER YOKE {num_edges} shoulder edges")
    garment = p.mesh_sections()
    # The surface inside the original bib has no body vertices to transfer
    # from. Interpolate its known seam weights into a neck-following collar.
    # This prevents the raised pistol arm from pulling one side of the neck.
    body = tailor.body
    body.transfer_bone_weights_from_mesh(garment)
    bones = {str(b.name): i for i, b in enumerate(body.get_all_bones_info()[-1])}
    for index, point in enumerate(p.by_mat["FieldKhaki"][0]):
        if point not in seam_weights:
            continue
        source, t = seam_weights[point]
        t = t * t * (3 - 2 * t)
        weights = defaultdict(float)
        for w in body.get_vertex_bone_weights(source)[-2]:
            weights[w.bone_index] += (1 - t) * w.weight
        weights[bones["spine_05"]] += 0.40 * t
        weights[bones["neck_01"]] += 0.60 * t
        weighted = [u.GeometryScriptBoneWeight(k, w) for k, w in weights.items() if w > 1.0e-7]
        if not garment.set_vertex_bone_weights(index, weighted)[-1]:
            raise RuntimeError(f"Invalid stand-collar vertex {index}")
    return garment


def riding_boot(tailor, side):
    p, s = tailor.panels, tailor.s
    foot, knee = tailor.b["foot_" + side], tailor.b["calf_" + side]

    def shaft(t, j):
        z = sew.mix(foot[2] - 0.25 * s, knee[2] - 6 * s, t)
        p0 = sew.lerp(foot, knee, (z - foot[2]) / (knee[2] - foot[2]))
        a = TAU * j
        n = sew.circular(a)
        d = tailor.radius(p0, n, 4.2 * s, 2.8 * s, 7.1 * s)
        ripple = (0.24 * sin(30 * t + cos(a)) * sew.bell(t, 0.15, 0.15) + 0.15 * sin(3 * a)) * s
        return sew.lift(p0, n, d + 2.25 * s + ripple)

    p.grid(23, 36, shaft, "RidingLeather")
    p.cord([shaft(1, j / 40) for j in range(41)], 0.37 * s, "RidingLeather")


def build():
    assets.make_directory(ROOT)
    body = assets.load_asset(sew.BODY)
    mesh = u.DynamicMesh()
    _, outcome = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        body, mesh, u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD()
    )
    if outcome != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Unable to fit the campaign tunic to its body")
    pos = [
        sew.tup(v)
        for v in u.GeometryScript_List.convert_vector_list_to_array(
            mesh.get_all_vertex_positions(False)[-2]
        )
    ]
    tri = [
        (v.x, v.y, v.z)
        for v in u.GeometryScript_List.convert_triangle_list_to_array(
            mesh.get_all_triangle_indices(False)[-2]
        )
    ]
    tailor = sew.Tailor(mesh)
    tailor.panels = CampaignPanels()
    coat = continuous_tunic(tailor, pos, tri)
    cover = make_coat(tailor, coat)
    for side in (-1, 1):
        bandolier(tailor, cover, side)
    # Fasteners and bandoliers ride directly on the tunic. Transfer its skin
    # weights so raising the pistol cannot pull a strap into the upper arm.
    harness = tailor.panels.mesh_sections()
    coat.transfer_bone_weights_from_mesh(harness)
    tailor.panels = CampaignPanels()
    # The trousers tuck into the boot tops. Trim the hidden calf fabric so it
    # cannot poke through the leather during the bandit's walk or collapse.
    inside_boot = min(tailor.b["calf_l"][2], tailor.b["calf_r"][2]) - 9 * tailor.s
    tailor.pants(pos, [f for f in tri if max(pos[i][2] for i in f) >= inside_boot])
    for side in ("l", "r"):
        tailor.sleeve(side)
        tailor.shoe(side, pos)
        riding_boot(tailor, side)
    cloth = tailor.panels.mesh_sections()
    mesh.transfer_bone_weights_from_mesh(cloth)
    cloth.append_mesh(coat, u.Transform())
    cloth.append_mesh(harness, u.Transform())
    for i in range(0, cloth.get_vertex_count(), 29):
        weights, valid = cloth.get_vertex_bone_weights(i)[-2:]
        if not valid or not weights or abs(sum(w.weight for w in weights) - 1) > 0.01:
            raise RuntimeError(f"Invalid campaign wardrobe skin weights at vertex {i}")
    outfit = sew.save_mesh(cloth, "SK_BanditCampaignDress", body.skeleton, materials(), ROOT)
    if len(outfit.materials) != len(SWATCHES):
        raise RuntimeError("Missing bandit wardrobe material slot")
    u.log(
        f"BANDIT DRESS READY {cloth.get_vertex_count()} vertices, bounds {cloth.get_mesh_bounding_box()}"
    )


if __name__ == "__main__":
    build()
