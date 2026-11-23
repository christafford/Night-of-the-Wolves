"""Fit the hat, moustache and swept hair to the exported head in rest-pose space."""

from math import atan2, cos, exp, pi, sin
from random import Random

import unreal as u
from bandit.head import ROOT
from hermit.appearance import SurfaceMesh, compile_with_renderer, save

assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary
TAU = 2 * pi


def material(name, hair=False):
    path = ROOT + "/" + name
    if assets.does_asset_exist(path):
        mat = assets.load_asset(path)
    else:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(
            name, ROOT, u.Material, u.MaterialFactoryNew()
        )
    existing = list(edit.get_material_expressions(mat))

    def node(cls):
        for idx, item in enumerate(existing):
            if isinstance(item, cls):
                return existing.pop(idx)
        return edit.create_material_expression(mat, cls)

    mat.set_editor_property("two_sided", True)
    vertex = node(u.MaterialExpressionVertexColor)
    uv = node(u.MaterialExpressionTextureCoordinate)

    def shader(label, code, inputs, pin, output):
        n = node(u.MaterialExpressionCustom)
        pins = []
        for k, src, channel in inputs:
            p = u.CustomInput()
            p.set_editor_property("input_name", k)
            pins.append(p)
        n.set_editor_properties(
            {"description": label, "code": code, "inputs": pins, "output_type": output}
        )
        for k, src, channel in inputs:
            if not edit.connect_material_expressions(src, channel, n, k):
                raise RuntimeError("Cannot wire " + label + " input " + k)
        edit.connect_material_property(n, "", pin)

    scalar = u.CustomMaterialOutputType.CMOT_FLOAT1
    rgb = u.CustomMaterialOutputType.CMOT_FLOAT3
    if hair:
        mat.set_editor_properties(
            {"blend_mode": u.BlendMode.BLEND_MASKED, "opacity_mask_clip_value": 0.29}
        )
        sample = node(u.MaterialExpressionTextureSample)
        sample.set_editor_properties(
            {
                "texture": assets.load_asset(
                    "/Game/NightOfTheWolves/FirstLeg/Hermit/T_WhiskerStrands"
                ),
                "sampler_type": u.MaterialSamplerType.SAMPLERTYPE_MASKS,
            }
        )
        shader(
            "Fine hairs over dense dark roots",
            "return lerp(1., Atlas.r, V.g);",
            [("V", vertex, ""), ("Atlas", sample, "RGB")],
            u.MaterialProperty.MP_OPACITY_MASK,
            scalar,
        )
        shader(
            "Warm black strand variation",
            """
float w=saturate(.79*V.r+.15*Atlas.g+.07*Atlas.b);
return lerp(float3(.006,.0045,.0035),float3(.075,.047,.031),w*w);
""",
            [("V", vertex, ""), ("Atlas", sample, "RGB")],
            u.MaterialProperty.MP_BASE_COLOR,
            rgb,
        )
    else:
        shader(
            "Worn felt and bound edge",
            """
float2 f=uv*float2(600,370);
float2 w=saturate(1.-fwidth(f));
float fuzz=.038*dot(w,sin(f*6.28));
return V*(1.+fuzz+.05*cos(uv.x*91.+sin(uv.y*27.))*sin(uv.y*29.));
""",
            [("V", vertex, ""), ("uv", uv, "")],
            u.MaterialProperty.MP_BASE_COLOR,
            rgb,
        )
    for pin, v in [
        (u.MaterialProperty.MP_ROUGHNESS, 0.78 if hair else 0.87),
        (u.MaterialProperty.MP_SPECULAR, 0.25),
    ]:
        n = node(u.MaterialExpressionConstant)
        n.set_editor_property("r", v)
        edit.connect_material_property(n, "", pin)
    compile_with_renderer(mat)
    save(mat)
    return mat


def make_hat(bind):
    brow = bind.get_bone_transform("FACIAL_C_Forehead", True).translation
    h = SurfaceMesh()
    # The oval underside is prominent in the supplied photograph. The crown
    # sits low behind it, with a shallow central dent and upturned bound edge.
    ox, oy, oz = brow.x, brow.y - 8.9, brow.z + 2.45

    def tip(x, y, z):
        tilt = 0.47
        return (
            ox + x + 0.020 * z,
            oy + y * cos(tilt) - z * sin(tilt) + 0.58,
            oz + y * sin(tilt) + z * cos(tilt) - 0.049 * x,
        )

    def crown(t, s, extra=0):
        a = TAU * s
        n, f = cos(a), sin(a)
        pinch = exp(-(((abs(n) - 0.52) / 0.28) ** 2)) * max(0, f) * t**2
        shape = 1 - 0.19 * t - 0.08 * t**3 - 0.085 * pinch
        return (
            (9.60 * shape + extra) * n - 0.27 * t,
            (11.38 * shape + extra) * f - 0.3 * t,
            0.17 * sin(a) + t * (9.45 + 1.05 * abs(n) - 0.34 * f),
        )

    def color(t, s):
        x = (
            0.91
            + 0.08 * sin(TAU * s * 3 + 0.3) * cos(2 * t)
            + 0.055 * sin(TAU * s * 9 + 3 * t)
        )
        return (0.052 * x, 0.043 * x, 0.032 * x)

    h.grid(18, 104, lambda t, s: tip(*crown(t, s)), color=color)

    def top(t, s):
        x, y, z = crown(1, s)
        k = 1 - 0.999 * t
        return tip(
            -0.27 + (x + 0.27) * k,
            -0.3 + (y + 0.3) * k,
            z * k + (1 - k) * 8.65 - 0.53 * sin(pi * t),
        )

    h.grid(13, 104, top, color=color)

    def brim(t, s, zoff=0):
        a = TAU * s
        x = cos(a) * (8.90 + 9.55 * t)
        y = sin(a) * (10.55 + 8.55 * t)
        lift = 1.04 + 0.72 * cos(a) ** 2 + 0.26 * sin(3 * a + 0.4)
        return tip(x, y, 0.14 * sin(a) - 0.16 - 0.88 * t + lift * t * t + zoff)

    h.grid(16, 128, brim, color=color)
    h.grid(
        12,
        128,
        lambda t, s: brim(t, s, -0.31),
        color=lambda t, s: (0.039, 0.031, 0.023),
        reverse=True,
    )

    def edge(t, s):
        return brim(1 + sin(pi * t) * 0.016, s, -0.31 * t)

    h.grid(6, 128, edge, color=lambda t, s: (0.058, 0.039, 0.026))
    h.grid(
        9,
        104,
        lambda t, s: tip(*crown(0.042 + 0.12 * t, s, 0.11)),
        color=lambda t, s: (0.042, 0.027, 0.016),
    )
    for i in range(2):

        def cord(t, s):
            a = TAU * (t + 2 * s)
            return tip(
                *crown(
                    0.058
                    + 0.064 * i
                    + 0.010 * cos(TAU * s * 37 + i * pi)
                    + 0.009 * cos(a),
                    s,
                    0.21 + 0.12 * sin(a),
                )
            )

        h.grid(6, 240, cord, color=lambda t, s: (0.166, 0.117, 0.066))
    h.save(
        "SM_VillaHat",
        material("M_VillaFelt"),
        bind.get_bone_transform("head", True),
        ROOT,
    )


def make_hair(bind, head):
    hair = SurfaceMesh()
    rng = Random(1914)
    skin = u.DynamicMesh()
    _, result = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        head,
        skin,
        u.GeometryScriptCopyMeshFromAssetOptions(),
        u.GeometryScriptMeshReadLOD(),
    )
    if result != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Cannot fit whiskers without the bandit's face")
    bvh = skin.build_bvh_for_mesh()[-1]
    query = u.GeometryScriptSpatialQueryOptions()

    def cast(origin, normal):
        v = u.Vector(*normal)
        hit = skin.find_nearest_ray_intersection_with_mesh(
            bvh, u.Vector(*origin) + v * 35, u.Vector(*[-n for n in normal]), query
        )[-2]
        if not hit.hit:
            raise RuntimeError(
                "Could not locate bandit skin at " + str((origin, normal))
            )
        return hit.hit_position

    lip = bind.get_bone_transform("FACIAL_C_LipUpper", True).translation
    brow = bind.get_bone_transform("FACIAL_C_Forehead", True).translation

    def face_y(x, z):
        return cast((x, lip.y, z), (0, 1, 0)).y

    def card(point, across, width, tone, col, steps=8):
        def surface(t, s):
            p = point(t)
            w = (s - 0.5) * width * (0.40 + 0.60 * min(t * 5, 1)) * (1 - 0.94 * t**3)
            return tuple(v + n * w for v, n in zip(p, across(t)))

        hair.grid(
            steps,
            2,
            surface,
            uv=lambda t, s: ((col * 128 + 9 + s * 110) / 1024, (14 + t * 480) / 512),
            color=lambda t, s: (tone * (0.58 + 0.42 * t), 1.0, t),
        )

    # A continuous foundation keeps the thick, split moustache readable at
    # gameplay distance. Both halves hug the upper lip and fall over the corners.
    for sign in (-1, 1):

        def foundation(t, s):
            a = TAU * s
            x = lip.x + sign * (-0.15 + 5.16 * t)
            mid = lip.z + 0.75 - 0.62 * t - 1.03 * t * t
            radius = (0.65 + 0.29 * sin(pi * t)) * (1 - 0.74 * t**3)
            z = mid + radius * sin(a) + 0.034 * sin(42 * t + 1.5 * s)
            return (x, face_y(x, z) + 0.035 + 0.27 * radius * cos(a), z)

        hair.grid(39, 17, foundation, color=lambda t, s: (0.24, 0, 0))
        for j in range(1550):
            start = rng.uniform(-0.47, 4.86)
            q = max(0.0, start / 5.18)
            x = lip.x + sign * start
            z = lip.z + 1.50 - 0.65 * q - 0.71 * q * q + rng.uniform(-0.78, 0.19)
            y = face_y(x, z) + 0.04
            span = min(5.42 - start, rng.uniform(1.15, 2.52))
            if span < 0.3:
                continue
            endx = x + sign * span
            endz = z - rng.uniform(0.94, 1.57) - 0.58 * q
            endy = face_y(endx, endz) + 0.18
            phase = rng.random() * TAU

            def path(t):
                return (
                    x + (endx - x) * t,
                    y + (endy - y) * t + 0.59 * sin(pi * t) + 0.19 * t,
                    z + (endz - z) * t**1.14 + 0.08 * t * sin(phase + 4 * t),
                )

            card(
                path,
                lambda t: (0, 0.12, -1),
                rng.uniform(0.050, 0.168),
                rng.uniform(0.20, 0.87),
                rng.randrange(0, 8),
                8,
            )
    # Swept-back hair grows along the high temples, sideburns and nape. Find
    # each root on the head, then comb short wavy locks towards the occiput.
    cx, cy = brow.x, brow.y - 9.4

    def hairline(a):
        f = cos(a)
        if f >= 0:
            return (
                brow.z
                - 3.3
                + 7.5 * f
                + 2.45 * exp(-(((abs(a) - 0.72) / 0.34) ** 2))
                - 0.66 * exp(-(((a - 0.12) / 0.23) ** 2))
            )
        return brow.z - 3.3 + 4.1 * f

    # A short combed undercoat fills the scalp between the individual wavy rows.
    # The lower hairline follows the skin, then the scalp curves naturally up
    # beneath the wide, tipped-back brim. Keep the visible ear clear.
    def undercoat(t, s):
        a = (s - 0.5) * TAU
        n, f = sin(a), cos(a)
        base_z = hairline(a) + 0.60 + 0.13 * sin(17 * a) + 0.09 * sin(31 * a + 0.3)
        # Lift the cap just above the ear rather than crossing its helix.
        ear = exp(-(((abs(a) - 1.55) / 0.38) ** 2))
        base_z += 4.0 * ear
        z_o = brow.z
        slope = atan2(base_z - z_o, 9.3)
        el = slope + (pi * 0.499 - slope) * t
        norm = (n * cos(el), f * cos(el), sin(el))
        q = cast((cx, cy, z_o), norm)
        puff = 0.16 + 0.32 * sin(t * pi) + 0.09 * sin(19 * a + 6 * t) * sin(pi * t)
        return tuple(q.to_tuple()[j] + puff * norm[j] for j in range(3))

    hair.grid(
        14,
        112,
        undercoat,
        color=lambda t, s: (0.25 + 0.09 * sin(s * TAU * 21 + 6 * t), 0.0, 0.0),
    )

    for j in range(2480):
        a = rng.uniform(-pi, pi)
        z = hairline(a) + rng.uniform(-0.12, 2.85)
        z += 4.0 * exp(-(((abs(a) - 1.55) / 0.38) ** 2))
        n, f = sin(a), cos(a)
        root = cast((cx, cy, z), (n, f, 0))
        x, y = root.x + n * 0.12, root.y + f * 0.12
        # Forelock sweeps across its shallow part. Side locks follow comb lines
        # diagonally behind the ears, so the face and outer ear remain exposed.
        side = 1 if a >= 0.15 else -1
        front = max(f, 0)
        run = rng.uniform(0.92, 2.45)
        lift = front * 2.1 + 0.30
        fall = (1 - front) * rng.uniform(1.2, 3.1)
        phase = rng.random() * TAU

        def path(t):
            dist = run * t
            return (
                x + side * f * dist + n * 0.40 * sin(pi * t),
                y - side * n * dist + f * 0.42 * sin(pi * t),
                z
                + (lift - fall) * t
                + 0.59 * sin(pi * t)
                + 0.18 * t * sin(phase + 6 * t),
            )

        card(
            path,
            lambda t: (0.19 * n, 0.19 * f, 1),
            rng.uniform(0.09, 0.31),
            rng.uniform(0.16, 0.80),
            rng.randrange(8),
            7,
        )
    hair.save(
        "SM_VillaHair",
        material("M_VillaHair", True),
        bind.get_bone_transform("head", True),
        ROOT,
    )


def build():
    head = assets.load_asset(ROOT + "/Villa_Head")
    bind = u.SkeletonModifier()
    if not bind.set_skeletal_mesh(head):
        raise RuntimeError("Build Villa_Head before fitting accessories")
    make_hat(bind)
    make_hair(bind, head)
