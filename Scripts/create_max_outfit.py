"""Tailor the trail clothes to the saved Max_Body from MetaHuman Creator.

From the project root, after running Scripts/max/create_max.py:

    "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Night_of_the_Wolves.uproject" \
        -run=pythonscript "-script=$PWD/Scripts/create_max_outfit.py" -EnablePlugins=GeometryScripting \
        -nullrhi -unattended -nosound

The cooked skeletal meshes in Max/Wardrobe contain all materials and skin weights.
"""

import math
import os

import unreal as u

DEST = "/Game/NightOfTheWolves/Max/Wardrobe"
BODY = os.environ.get("NOTW_MAX_BODY", "/Game/NightOfTheWolves/Max/Anatomy/Max_Body")
assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
sin, cos, pi = math.sin, math.cos, math.pi

# Base color in linear space, roughness and the strength of the woven surface.
SWATCHES = {
    "IndigoShell": ((0.038, 0.075, 0.131), 0.76, 1.0),
    "OchreYoke": ((0.38, 0.234, 0.099), 0.80, 0.9),
    "HeatherJersey": ((0.49, 0.50, 0.485), 0.88, 0.65),
    "WashedDenim": ((0.025, 0.034, 0.051), 0.87, 1.2),
    "DarkBinding": ((0.014, 0.022, 0.035), 0.66, 0.55),
    "OxbloodCanvas": ((0.25, 0.042, 0.033), 0.73, 0.85),
    "Rubber": ((0.47, 0.43, 0.34), 0.67, 0.12),
    "Laces": ((0.255, 0.192, 0.135), 0.80, 0.5),
    "Hardware": ((0.19, 0.17, 0.13), 0.41, 0.03),
}


def mats():
    root = DEST + "/M_TrailFabric"
    mat = assets.load_asset(root) if assets.does_asset_exist(root) else None
    if mat is None:
        mat = tools.create_asset("M_TrailFabric", DEST, u.Material, u.MaterialFactoryNew())
        mat.set_editor_properties({"used_with_skeletal_mesh": True, "two_sided": True})

        def node(kind, x, y):
            return edit.create_material_expression(mat, kind, x, y)

        def param(name, default, y):
            n = node(u.MaterialExpressionScalarParameter, -520, y)
            n.set_editor_properties({"parameter_name": name, "default_value": default})
            return n

        tint = node(u.MaterialExpressionVectorParameter, -520, -160)
        tint.set_editor_properties(
            {"parameter_name": "Tint", "default_value": u.LinearColor(0.1, 0.1, 0.1, 1)}
        )
        rough = param("Roughness", 0.82, 260)
        weave = param("Weave", 1.0, 115)
        uv = node(u.MaterialExpressionTextureCoordinate, -520, 0)
        tint_fabric = node(u.MaterialExpressionCustom, -140, -80)
        pins = []
        for name in ("UV", "Tint", "Weave"):
            p = u.CustomInput()
            p.set_editor_property("input_name", name)
            pins.append(p)
        # Soften the woven pattern to its mean as its thread width falls below a pixel.
        tint_fabric.set_editor_properties(
            {
                "description": "Dyed cloth with filtered warp and weft",
                "output_type": u.CustomMaterialOutputType.CMOT_FLOAT3,
                "inputs": pins,
                "code": """
float2 P = UV * 1024.0;
float2 mask = saturate(1.0 - fwidth(P) * 1.35);
float detail = dot(mask * cos(P * 6.2831853), float2(.025,.017));
float slub = .019 * sin(dot(UV, float2(87,257))) * sin(dot(UV,float2(193,-69)));
return Tint.rgb * (1.0 + Weave * (detail + slub));""",
            }
        )
        for a, out, slot in [(uv, "", "UV"), (tint, "", "Tint"), (weave, "", "Weave")]:
            if not edit.connect_material_expressions(a, out, tint_fabric, slot):
                raise RuntimeError("Missing fabric input " + slot)
        edit.connect_material_property(tint_fabric, "", u.MaterialProperty.MP_BASE_COLOR)
        edit.connect_material_property(rough, "", u.MaterialProperty.MP_ROUGHNESS)
        assets.save_loaded_asset(mat, False)
    if not edit.get_material_property_input_node(mat, u.MaterialProperty.MP_NORMAL):
        # Soft oblique creases and filtered thread highlights change with the light,
        # including moonlight behind the shoulder. UVs travel with every garment.
        normal = edit.create_material_expression(mat, u.MaterialExpressionCustom, -140, 500)
        uv = edit.create_material_expression(mat, u.MaterialExpressionTextureCoordinate, -520, 440)
        amount = edit.create_material_expression(mat, u.MaterialExpressionScalarParameter, -520, 600)
        amount.set_editor_properties({"parameter_name": "Weave", "default_value": 1.0})
        pins = []
        for name in ("UV", "Strength"):
            pin = u.CustomInput()
            pin.set_editor_property("input_name", name)
            pins.append(pin)
        normal.set_editor_properties(
            {
                "description": "Fine twill and irregular drape ridges",
                "inputs": pins,
                "output_type": u.CustomMaterialOutputType.CMOT_FLOAT3,
                "code": """
float2 p = UV * float2(470,590);
float2 fade = saturate(1.0-fwidth(p)*1.25);
float2 thread = .085*fade*sin(6.2831853*(p+float2(.23*p.y,0)));
float x=UV.x, y=UV.y;
float pinch=.30+.70*pow(abs(cos(6.28*y+1.2*sin(6.28*x))),2);
float ripple=sin(69*y + 3.4*sin(7*x+1.1)+2.0*sin(12*x-7*y));
float bias=sin(38*y - 14*x + 2.5*sin(9*x+4*y));
float2 folds=pinch*float2(.027*bias,.088*ripple+.043*bias);
return normalize(float3((thread+folds)*Strength,1.0));""",
            }
        )
        edit.connect_material_expressions(uv, "", normal, "UV")
        edit.connect_material_expressions(amount, "", normal, "Strength")
        edit.connect_material_property(normal, "", u.MaterialProperty.MP_NORMAL)
        assets.save_loaded_asset(mat, False)
    slots = {}
    for name, (color, rough, weave) in SWATCHES.items():
        path = DEST + "/MI_" + name
        mi = assets.load_asset(path) if assets.does_asset_exist(path) else None
        if mi is None:
            mi = tools.create_asset(
                "MI_" + name, DEST, u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew()
            )
        edit.set_material_instance_parent(mi, mat)
        edit.set_material_instance_vector_parameter_value(mi, "Tint", u.LinearColor(*color, 1))
        edit.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
        edit.set_material_instance_scalar_parameter_value(mi, "Weave", weave)
        assets.save_loaded_asset(mi, False)
        slots[name] = mi
    return slots


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def scale(a, s):
    return tuple(x * s for x in a)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def unit(a):
    return scale(a, 1 / max(1e-8, math.sqrt(dot(a, a))))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def lerp(a, b, t):
    return add(a, scale(sub(b, a), t))


def mix(a, b, t):
    return a + (b - a) * t


def clamp(x, lo, hi):
    return max(lo, min(hi, x))


def bell(x, c, w):
    return math.exp(-(((x - c) / w) ** 2))


def circular(t):
    return (sin(t), cos(t), 0)


def lift(p, direction, amount):
    return add(p, scale(direction, amount))


def tup(v):
    return (v.x, v.y, v.z)


class Panels:
    def __init__(self):
        self.by_mat = {m: [[], [], [], {}] for m in SWATCHES}

    def tri(self, m, points, uv):
        v, uvs, faces, indices = self.by_mat[m]
        face = []
        for p, coord in zip(points, uv):
            key = tuple(round(a, 5) for a in (*p, *coord))
            if key not in indices:
                indices[key] = len(v)
                v.append(p)
                uvs.append(coord)
            face.append(indices[key])
        if len(set(face)) == 3:
            faces.append(face)

    def grid(self, rows, steps, at, style, flip=False):
        # The grid order is fixed across the seam to preserve manifold skin-weight transfer.
        pts = [[at(i / (rows - 1), j / steps) for j in range(steps + 1)] for i in range(rows)]
        for i in range(rows - 1):
            for j in range(steps):
                s = style((i + 0.5) / (rows - 1), (j + 0.5) / steps) if callable(style) else style
                if s is None:
                    continue
                for tri in [((i, j), (i, j + 1), (i + 1, j + 1)), ((i, j), (i + 1, j + 1), (i + 1, j))]:
                    if flip:
                        tri = tri[::-1]
                    self.tri(s, [pts[k][n] for k, n in tri], [(n / steps, k / (rows - 1)) for k, n in tri])

    def patched_sleeve(self, at, overlay):
        # Intersect patch contours in sleeve UV space. Both fabrics then deform on
        # a single skin surface, including across the bent elbow.
        rows, columns = 40, 48
        cache = {}

        def surface(uv):
            if uv not in cache:
                cache[uv] = at(*uv)
            return cache[uv]

        def clip(polygon, field):
            inside, outside = [], []
            for start, end in zip(polygon[-1:] + polygon[:-1], polygon):
                a, b = field(*start), field(*end)
                if (a >= 0) != (b >= 0):
                    hit = lerp(start, end, clamp(a / (a - b), 0, 1))
                    inside.append(hit)
                    outside.append(hit)
                (inside if b >= 0 else outside).append(end)
            return inside, outside

        def emit(poly, material):
            for k in range(1, len(poly) - 1):
                uv = [poly[0], poly[k], poly[k + 1]]
                self.tri(material, [surface(q) for q in uv], [(j, t) for t, j in uv])

        for i in range(rows):
            for j in range(columns):
                a, b, c, d = [
                    (x / rows, y / columns) for x, y in ((i, j), (i, j + 1), (i + 1, j + 1), (i + 1, j))
                ]
                for poly in [[c, b, a], [d, c, a]]:
                    if (i + 0.5) / rows > 0.925:
                        emit(poly, "DarkBinding")
                        continue
                    tan, blue = clip(poly, overlay)
                    emit(tan, "OchreYoke")
                    emit(blue, "IndigoShell")

    def cord(self, path, r, m):
        frames = []
        for i, p in enumerate(path):
            axis = unit(sub(path[min(len(path) - 1, i + 1)], path[max(0, i - 1)]))
            a = unit(cross(axis, (1, 0, 0) if abs(axis[0]) < 0.8 else (0, 1, 0)))
            frames.append((p, a, cross(axis, a)))

        def pt(i, t):
            p, a, b = frames[round(i * (len(path) - 1))]
            return add(p, add(scale(a, r * cos(t * 2 * pi)), scale(b, r * sin(t * 2 * pi))))

        self.grid(len(path), 7, pt, m, True)

    def dynamic(self):
        target = u.DynamicMesh()
        for mid, (m, (v, uvs, faces, _)) in enumerate(self.by_mat.items()):
            if not faces:
                raise RuntimeError("Unassigned garment material " + m)
            buffers = u.GeometryScriptSimpleMeshBuffers()
            buffers.set_editor_properties(
                {
                    "vertices": [u.Vector(*p) for p in v],
                    "triangles": [u.IntVector(*t) for t in faces],
                    "uv0": [u.Vector2D(*p) for p in uvs],
                }
            )
            target.append_buffers_to_mesh(buffers, mid)
        target.set_per_vertex_normals()
        return target


class Tailor:
    def __init__(self, body):
        self.body = body
        self.bvh = body.build_bvh_for_mesh()[-1]
        self.query = u.GeometryScriptSpatialQueryOptions()
        self.b = {str(b.name): tup(b.world_transform.translation) for b in body.get_all_bones_info()[-1]}
        self.panels = Panels()
        self.s = self.b["neck_01"][2] / 147.0
        self.hip = self.b["pelvis"][2]
        self.neck = self.b["neck_01"]
        self.shoulder = self.b["upperarm_l"]
        self.hem = self.hip - 7.0 * self.s
        self.chest = self.shoulder[2] - 11.5 * self.s

    def radius(self, p, normal, fallback, lo, hi):
        hit = self.body.find_nearest_ray_intersection_with_mesh(
            self.bvh, u.Vector(*p), u.Vector(*normal), self.query
        )[-2]
        return clamp(hit.ray_parameter if hit.hit else fallback, lo, hi)

    def torso(self, z, a, allowance):
        s = self.s
        n = circular(a)
        # Cast from inside each cross section. The lateral bound separates cloth from
        # the neighbouring arm at armpit height.
        cy = mix(
            self.b["pelvis"][1], self.b["spine_05"][1], clamp((z - self.hem) / (self.chest - self.hem), 0, 1)
        )
        p = (0, cy, z)
        shoulder_width = abs(self.shoulder[0])
        wide = mix(
            shoulder_width * 1.01,
            shoulder_width * 1.22,
            clamp((z - self.hem) / (self.chest - self.hem), 0, 1),
        )
        d = self.radius(p, n, 13 * s, 5 * s, min(23 * s, wide / max(abs(n[0]), 0.1)))
        belly = clamp((self.chest - z) / (self.chest - self.hem), 0, 1)
        # Let the shell fall from the chest and clear the waist instead of following bare anatomy.
        d = max(d, 1 / math.sqrt((n[0] / (wide * 0.92)) ** 2 + (n[1] / (9.2 * s)) ** 2))
        drape = (
            allowance * (1 + 0.18 * sin(8 * a + 0.06 * z) + 0.11 * sin(13 * a - 0.025 * z)) + belly * 0.5 * s
        )
        return lift(p, n, d + drape)

    def jacket(self):
        s = self.s

        def lower(t, a, shirt=False):
            z = mix(self.hem + ((1.6 - 3.3 * max(0, cos(a)) ** 2) * s if shirt else 0), self.chest, t)
            z += (1 - t) ** 3 * (0.65 * sin(3 * a + 0.5) - 0.7 * cos(a)) * s
            p = self.torso(z, a, (0.85 if shirt else 3.25) * s)
            if not shirt:
                # Draped cloth hangs in broad vertical folds through the midsection.
                swing = 0.46 * sin(5 * a + 0.55) + 0.28 * sin(9 * a + 0.6 * t) + 0.12 * cos(16 * a - 0.9 * t)
                p = lift(p, circular(a), swing * s * max(0, sin(t * pi)) ** 0.74)
            crease = bell(t, 0.13, 0.14) * (0.7 * sin(13 * t + 3 * a) + 0.4 * cos(25 * t - 4 * a)) * s
            return lift(p, circular(a), crease * (0.22 if shirt else 1))

        def top(t, a, shirt=False):
            start = lower(1, a, shirt)
            # Shoulder seams rise toward the neck. The back collar stands higher
            # than the split front lapel and the shirt's crew neck.
            near_y = self.neck[1]
            rx = (5.9 if shirt else 8.3) * s
            ry = (5.8 if shirt else 7.6) * s
            final = (
                rx * sin(a),
                near_y + ry * cos(a),
                self.neck[2] + s * ((-0.1 if shirt else 2.6) - max(0, cos(a)) * (4.5 if shirt else 3.0)),
            )
            if t <= 0.57:
                k = t / 0.57
                shoulder = self.shoulder
                ridge = (
                    sin(a) * (abs(shoulder[0]) + 4.2 * s),
                    near_y + cos(a) * s * (9.8 + 1.3 * max(0, cos(a))),
                    shoulder[2] + s * (4.7 - 4.8 * max(0, cos(a)) ** 2 - 1.0 * abs(cos(a))),
                )
                if shirt:
                    ridge = (ridge[0] * 0.82, mix(near_y, ridge[1], 0.84), ridge[2] - 2.0 * s)
                return lerp(start, ridge, k)
            k = (t - 0.57) / 0.43
            mid = top(0.57, a, shirt)
            p = lerp(mid, final, k)
            return add(p, (0, 0, s * 0.8 * sin(k * pi)))

        def angle(t, j):
            half = 0.34 + 0.21 * (1 - t) ** 2 + 0.14 * t**3
            return mix(half, 2 * pi - half, j)

        def garment(t, j, shirt=False):
            a = 2 * pi * j if shirt else angle(t, j)
            return lower(t / 0.64, a, shirt) if t <= 0.64 else top((t - 0.64) / 0.36, a, shirt)

        self.panels.grid(
            31,
            72,
            lambda t, j: garment(t, j, True),
            lambda t, j: "HeatherJersey" if cos(2 * pi * j) > 0.46 or t > 0.978 else None,
        )

        def jacket_panel(t, j):
            # Put a row of actual panel vertices on the curved yoke seam.
            boundary = 0.75 + 0.067 * cos(2 * pi * j) ** 2 + 0.014 * cos(4 * pi * j)
            mapped = mix(0, boundary, t / 0.75) if t <= 0.75 else mix(boundary, 1, (t - 0.75) / 0.25)
            return garment(mapped, j)

        self.panels.grid(
            45,
            80,
            jacket_panel,
            lambda t, j: "DarkBinding" if t < 0.021 else "OchreYoke" if t > 0.75 else "IndigoShell",
        )

        # The rolled collar is a separate thicker folded piece whose underside is visible.
        def collar(t, j):
            a = angle(1, j)
            base = top(1, a, False)
            n = circular(a)
            h = 3.0 * s * (1 - 0.38 * cos(a))
            if t < 0.55:
                return add(lift(base, n, 0.95 * s * t / 0.55), (0, 0, h * t / 0.55))
            k = (t - 0.55) / 0.45
            return add(lift(base, n, (0.95 + 1.35 * k) * s), (0, 0, h * (1 - 0.88 * k)))

        self.panels.grid(8, 64, collar, "IndigoShell")
        for side in [0, 1]:
            self.panels.cord([garment(k / 44, side) for k in range(45)], 0.32 * s, "DarkBinding")
            # Two angled welt pockets on the lower fronts.
            path = []
            for i in range(12):
                k = i / 11
                t = 0.095 + 0.21 * k
                j = 0.062 + 0.03 * k
                p = garment(t, j if side == 0 else 1 - j)
                path.append(lift(p, circular(angle(t, j if side == 0 else 1 - j)), 0.25 * s))
            self.panels.cord(path, 0.42 * s, "DarkBinding")
        anchor = garment(0.1, 0)
        pull = [
            add(
                anchor, (0.23 * s * sin(2 * pi * k / 14), 0.40 * s, -s * (1.05 + 0.94 * cos(2 * pi * k / 14)))
            )
            for k in range(15)
        ]
        self.panels.cord(pull, 0.16 * s, "Hardware")
        for h in (0.25, 0.45, 0.64):
            x, y, z = garment(h, 0.005)
            self.panels.cord(
                [
                    (x + 0.30 * s * sin(k * 2 * pi / 12), y + 0.19 * s, z + 0.30 * s * cos(k * 2 * pi / 12))
                    for k in range(13)
                ],
                0.13 * s,
                "Hardware",
            )
        self.panels.cord([top(1, k * 2 * pi / 64, True) for k in range(65)], 0.43 * s, "HeatherJersey")

    def sleeve(self, side):
        sh, el, wr = [self.b[k + "_" + side] for k in ["upperarm", "lowerarm", "hand"]]
        s = self.s
        sign = 1 if side == "l" else -1

        # Follow the actual slightly bent Creator A-pose.
        def center(t):
            if t < 0.5:
                return lerp(sh, el, t * 2)
            return lerp(el, wr, (t - 0.5) * 2)

        def sleeve(t, j):
            a = 2 * pi * j
            axis = unit(sub(center(min(1, t + 0.02)), center(max(0, t - 0.02))))
            up = unit(sub((0, 0, 1), scale(axis, axis[2])))
            across = unit(cross(axis, up))
            n = add(scale(up, cos(a)), scale(across, sin(a)))
            p = center(t)
            radius = self.radius(p, n, (5.3 - 2 * t) * s, 2.2 * s, (7.1 - 2.9 * t) * s)
            ease = 1.65 * s + 0.7 * s * sin(t * pi)
            folds = (
                0.48
                * s
                * (bell(t, 0.53, 0.13) + bell(t, 0.82, 0.11))
                * sin(36 * t + 3 * cos(a) + 0.7 * sin(2 * a))
            )
            if t > 0.913:
                ease = mix(ease, 0.55 * s, (t - 0.913) / 0.087)
            # Dome the shoulder and bury its inner edge into the torso.
            if t < 0.105:
                ease -= 1.1 * s * (1 - t / 0.105)
            return lift(p, n, radius + ease + folds)

        elbow_center = 0.22 if sign > 0 else 0.78

        def patches(t, j):
            # Roll the pad around the outer elbow rather than across its inside crease.
            ca = cos(2 * pi * j)
            cap = min((0.158 + 0.080 * ca - t) * 6, ca + 0.49)
            oval = 1 - ((t - 0.55) / 0.108) ** 2 - ((j - elbow_center) / 0.14) ** 2
            return max(cap, oval)

        self.panels.patched_sleeve(sleeve, patches)
        seam = []
        for k in range(57):
            a = 2 * pi * k / 56
            t = 0.55 + 0.102 * cos(a)
            j = elbow_center + 0.131 * sin(a)
            surface = sleeve(t, j)
            seam.append(lift(surface, unit(sub(surface, center(t))), 0.055 * s))
        self.panels.cord(seam, 0.075 * s, "OchreYoke")
        self.panels.cord([sleeve(0.942, j / 46) for j in range(47)], 0.24 * s, "IndigoShell")

    def pants(self, pos, tri):
        s = self.s
        waist = self.hip + 8 * s
        ankle = self.b["foot_l"][2] + 0.95 * s

        def permitted(p):
            # Ignore hands that hang at hip height.
            return ankle - 2.4 * s < p[2] < waist + 2 * s and abs(p[0]) < abs(self.shoulder[0]) + 9 * s

        def denim(p):
            x, y, z = p
            side = "l" if x > 0 else "r"
            hip, knee, foot = [self.b[k + "_" + side] for k in ("thigh", "calf", "foot")]
            if z < knee[2]:
                ctr = lerp(foot, knee, clamp((z - foot[2]) / (knee[2] - foot[2]), 0, 1))
            else:
                ctr = lerp(knee, hip, clamp((z - knee[2]) / (hip[2] - knee[2]), 0, 1))
            merge = clamp((z - (hip[2] - 10 * s)) / (10 * s), 0, 1)
            cx = ctr[0] * (1 - merge)
            delta = (x - cx, y - ctr[1], 0)
            n = unit(delta)
            a = math.atan2(delta[0], delta[1])
            d = 1.0 * s + 1.6 * s * bell(z, ankle + 4 * s, 14 * s)
            # Bridge the hollow below each kneecap in this relaxed denim cut.
            d += 0.75 * s * bell(z, knee[2] - 5 * s, 6 * s)
            folds = (
                (bell(z, knee[2], 6 * s) + bell(z, ankle + 5 * s, 6 * s))
                * 0.25
                * s
                * cos(z / s * 0.81 + 2 * sin(a))
            )
            # Opposing pant legs meet at the crotch and may overlap slightly above it.
            p = lift((x, y, clamp(z, ankle, waist)), n, d + folds)
            return p, ((a + pi) / (2 * pi), z / (waist - ankle))

        for face in tri:
            pts = [pos[i] for i in face]
            if not all(permitted(p) for p in pts):
                continue
            if max(p[2] for p in pts) <= ankle or min(p[2] for p in pts) >= waist:
                continue
            fitted = [denim(p) for p in pts]
            self.panels.tri("WashedDenim", [p for p, _ in fitted], [uv for _, uv in fitted])

    def shoe(self, side, pos):
        s = self.s
        foot = self.b["foot_" + side]
        ball = self.b["ball_" + side]
        forward = unit((ball[0] - foot[0], ball[1] - foot[1], 0))
        right = (forward[1], -forward[0], 0)
        below = [sub(p, foot) for p in pos if p[2] < foot[2] + 2 * s and p[0] * foot[0] > 0]
        heel = min(dot(p, forward) for p in below) - 0.85 * s
        toe = max(dot(p, forward) for p in below) + 0.85 * s
        sole = min(p[2] + foot[2] for p in below) - 0.36 * s
        # Longitudinal last: the toe domes gradually while the heel counter stands upright.
        stations = [
            (0, 0.50, 0.58),
            (0.047, 0.73, 0.87),
            (0.16, 0.83, 1),
            (0.31, 0.90, 1.06),
            (0.43, 0.94, 1.00),
            (0.57, 1.01, 0.82),
            (0.71, 1.05, 0.61),
            (0.83, 1, 0.49),
            (0.93, 0.86, 0.43),
            (0.978, 0.49, 0.24),
            (0.992, 0.26, 0.115),
            (1, 0.012, 0.007),
        ]

        def shape(t):
            for (a, w0, h0), (b, w1, h1) in zip(stations, stations[1:]):
                if t <= b:
                    k = (t - a) / (b - a)
                    return mix(w0, w1, k), mix(h0, h1, k)

        def center(t):
            return add((foot[0], foot[1], sole), scale(forward, mix(heel, toe, t)))

        last_width = max(4.5 * s, max(abs(dot(v, right)) for v in below) * 0.93)

        def dimensions(t):
            w, h = shape(t)
            return last_width * w, h * 8.85 * s

        def camber(t):
            return 0.85 * s * clamp((t - 0.7) / 0.3, 0, 1) ** 2

        def upper(t, j):
            a = pi * j
            w, h = dimensions(t)
            return add(
                lift(center(t), right, w * cos(a)), (0, 0, 1.85 * s + camber(t) + h * max(0, sin(a)) ** 0.65)
            )

        self.panels.grid(39, 26, upper, "OxbloodCanvas")
        # Three rings around a raised, rounded rubber foxing. Elliptical ends close the last.
        outline = []
        for j in range(41):
            t = j / 40
            outline.append(lift(center(t), right, dimensions(t)[0]))
        for j in range(40, -1, -1):
            t = j / 40
            outline.append(lift(center(t), right, -dimensions(t)[0]))
        outline.append(outline[0])

        def sole_at(k, j):
            i = round(j * (len(outline) - 1))
            p = outline[i]
            t = (i if i <= 40 else 81 - i) / 40
            t = clamp(t, 0, 1)
            inside = sub(p, center(t))
            p = add(center(t), scale(inside, 1 + 0.035 * sin(k * pi)))
            return add(p, (0, 0, camber(t) + (0.05 + 1.86 * k) * s))

        self.panels.grid(5, len(outline) - 1, sole_at, "Rubber", True)
        self.panels.cord(
            [sole_at(0.77, j / (len(outline) - 1)) for j in range(len(outline))], 0.115 * s, "DarkBinding"
        )
        # Toe and heel caps close the upper and a flattish underside closes the shoe.
        for t in [0, 1]:
            self.panels.grid(
                3,
                26,
                lambda k, j: lerp(add(center(t), (0, 0, 1.84 * s + camber(t))), upper(t, j), k),
                "OxbloodCanvas",
                t == 1,
            )
        self.panels.grid(
            2, 40, lambda k, t: lerp(sole_at(0, t * 40 / 82), sole_at(0, (81 - 40 * t) / 82), k), "Rubber"
        )
        for i in range(4):
            t = 0.405 + i * 0.06
            # Paired diagonals give the canvas sneakers an actual four-row lacing.
            for a, b in [(0.32, 0.68), (0.68, 0.32)]:
                lace = [add(upper(t + 0.06 * k / 7, mix(a, b, k / 7)), (0, 0, 0.2 * s)) for k in range(8)]
                self.panels.cord(lace, 0.20 * s, "Laces")
        for side_edge in [0.27, 0.73]:
            eyestay = []
            for k in range(16):
                p = upper(0.365 + 0.32 * k / 15, side_edge)
                eyestay.append(add(p, (0, 0, 0.12 * s)))
            self.panels.cord(eyestay, 0.21 * s, "OxbloodCanvas")
        for j in [0.24, 0.76]:
            self.panels.cord(
                [add(upper(0.16 + 0.49 * k / 18, j), (0, 0, 0.08 * s)) for k in range(19)],
                0.16 * s,
                "OxbloodCanvas",
            )


def save_mesh(dynamic, name, skeleton, slots, folder=DEST):
    path = folder + "/" + name
    if assets.does_asset_exist(path):
        sk = assets.load_asset(path)
        if sk.skeleton != skeleton:
            raise RuntimeError("Existing outfit and authored body skeletons differ: " + path)
        opts = u.GeometryScriptCopyMeshToAssetOptions()
        opts.set_editor_properties(
            {
                "replace_materials": True,
                "new_materials": list(slots.values()),
                "new_material_slot_names": list(slots),
                "enable_recompute_tangents": True,
                "bone_hierarchy_mismatch_handling": u.GeometryScriptBoneHierarchyMismatchHandling.CREATE_NEW_REFERENCE_SKELETON,
            }
        )
        _, outcome = u.GeometryScript_AssetUtils.copy_mesh_to_skeletal_mesh(
            dynamic, sk, opts, u.GeometryScriptMeshWriteLOD()
        )
    else:
        opts = u.GeometryScriptCreateNewSkeletalMeshAssetOptions()
        opts.set_editor_properties(
            {"materials": slots, "use_mesh_bone_proportions": True, "enable_recompute_tangents": True}
        )
        sk, outcome = u.GeometryScript_NewAssetUtils.create_new_skeletal_mesh_asset_from_mesh(
            dynamic, skeleton, path, opts
        )
    if not sk or outcome != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Saving mesh failed: " + path)
    if not assets.save_loaded_asset(sk, False):
        raise RuntimeError("Could not persist mesh " + path)
    u.log(
        "MAX_WARDROBE_SAVED %s, %s triangles, %s material slots"
        % (path, dynamic.get_triangle_count(), len(sk.materials))
    )
    return sk


def main():
    body = u.load_asset(BODY)
    if not isinstance(body, u.SkeletalMesh):
        raise RuntimeError("Export Max_Body through Creator before fitting clothing")
    mesh = u.DynamicMesh()
    _, outcome = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        body, mesh, u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD()
    )
    if outcome != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Could not read Creator body geometry")
    pos = [
        tup(v)
        for v in u.GeometryScript_List.convert_vector_list_to_array(mesh.get_all_vertex_positions(False)[-2])
    ]
    tri = [
        (v.x, v.y, v.z)
        for v in u.GeometryScript_List.convert_triangle_list_to_array(
            mesh.get_all_triangle_indices(False)[-2]
        )
    ]
    tailor = Tailor(mesh)
    tailor.jacket()
    tailor.pants(pos, tri)
    for side in ["l", "r"]:
        tailor.sleeve(side)
        tailor.shoe(side, pos)
    cloth = tailor.panels.dynamic()
    mesh.transfer_bone_weights_from_mesh(cloth)
    for vid in range(0, cloth.get_vertex_count(), 41):
        weights, valid = cloth.get_vertex_bone_weights(vid)[-2:]
        if not valid or not weights or abs(sum(w.weight for w in weights) - 1) > 0.01:
            raise RuntimeError("Invalid clothing weights, vertex " + str(vid))
    u.log(
        "MAX_WARDROBE_FIT body=%s, %s triangles, %s bones"
        % (BODY, cloth.get_triangle_count(), len(cloth.get_all_bones_info()[-1]))
    )
    if os.environ.get("NOTW_MAX_FIT_PREVIEW"):
        import json

        with open(os.environ["NOTW_MAX_FIT_PREVIEW"], "w") as f:
            json.dump({k: [v[0], v[2], SWATCHES[k][0]] for k, v in tailor.panels.by_mat.items()}, f)
    if os.environ.get("NOTW_MAX_DRYRUN"):
        return
    assets.make_directory(DEST)
    save_mesh(cloth, "SK_MaxTrailOutfit", body.skeleton, mats())
    # The face mesh includes the lower neck. Keep both hands and additional forearm
    # beneath the sleeve cuffs. The full Creator-exported body is preserved separately.
    keep = [False] * len(pos)
    for side in ["l", "r"]:
        wrist = tailor.b["hand_" + side]
        elbow = tailor.b["lowerarm_" + side]
        along = unit(sub(wrist, elbow))
        for i, p in enumerate(pos):
            if (
                p[0] * wrist[0] > 0
                and abs(p[0]) > abs(wrist[0]) * 0.72
                and dot(sub(p, wrist), along) > -4 * tailor.s
            ):
                keep[i] = True
    cut = [i for i, t in enumerate(tri) if not any(keep[v] for v in t)]
    full_bounds = mesh.get_mesh_bounding_box()
    mesh.delete_triangles_from_mesh(u.GeometryScript_List.convert_array_to_index_list(cut))
    skin = {str(slot.material_slot_name): slot.material_interface for slot in body.materials}
    exposed = save_mesh(mesh, "SK_MaxExposedBody", body.skeleton, skin)
    fitted = mesh.get_mesh_bounding_box()
    # GetMesh drives the entire outfit. Its visibility bounds must include the full
    # figure even though its remaining triangles cover the bare hands alone.
    exposed.set_editor_properties(
        {
            "positive_bounds_extension": u.Vector(
                *[max(0, b - a) for a, b in zip(tup(fitted.max), tup(full_bounds.max))]
            ),
            "negative_bounds_extension": u.Vector(
                *[max(0, a - b) for a, b in zip(tup(fitted.min), tup(full_bounds.min))]
            ),
        }
    )
    assets.save_loaded_asset(exposed, False)


if __name__ == "__main__":
    main()
