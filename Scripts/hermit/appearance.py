"""Weathered head, dented felt hat, and salt-and-pepper whiskers for the resident."""

from math import cos, exp, pi, sin
from random import Random

import unreal as u

ROOT = "/Game/NightOfTheWolves/FirstLeg/Hermit"
MAX = "/Game/NightOfTheWolves/Max"
MH = "/MetaHumanCharacter"
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
edit = u.MaterialEditingLibrary


def save(obj):
    if not obj or not assets.save_loaded_asset(obj, False):
        raise RuntimeError("Could not save " + str(obj))


def compile_with_renderer(mat):
    if "-nullrhi" not in u.SystemLibrary.get_command_line().lower():
        problems = edit.recompile_material(mat)
        if problems:
            raise RuntimeError(str(problems))


def duplicate(source, name, folder=ROOT):
    path = folder + "/" + name
    return (
        assets.load_asset(path)
        if assets.does_asset_exist(path)
        else tools.duplicate_asset(name, folder, u.load_asset(source))
    )


def build_face():
    creator = u.get_editor_subsystem(u.MetaHumanCharacterEditorSubsystem)
    path = ROOT + "/Creator/Hermit"
    fresh = not assets.does_asset_exist(path)
    character = duplicate(MH + "/Optional/Presets/Walter", "Hermit", ROOT + "/Creator")
    if not creator.try_add_object_to_edit(character):
        raise RuntimeError("Close the hermit's character editor before rebuilding.")
    try:
        if fresh:
            character.get_editor_property("assembly_settings").set_editor_property(
                "name_override", "Hermit"
            )
            # UE 5.9 rig landmarks. X spans the face; positive Y goes toward the nose.
            # Final pairs slide the ear helix outward, and tip the lower ear down.
            marks = creator.get_face_landmarks(character)
            if len(marks) != 91:
                raise RuntimeError("Review Walter's landmark table for this engine version.")
            offsets = {}
            for left, right, dx, dy, dz in [
                (1, 28, 0.73, -0.28, -0.46),
                (2, 29, 0.52, -0.63, -0.48),
                (63, 43, 0.40, -0.12, -0.52),
                (3, 30, 0.30, 0.06, -0.42),
                (66, 23, 0.42, -0.62, -0.13),
                (58, 41, 0.10, 0.33, 0.24),
                (77, 24, 0.09, 0.35, -0.23),
                (10, 26, 0.16, 0.35, -0.16),
                (78, 25, 0.07, 0.16, 0.11),
                (67, 45, 0.22, -0.05, -0.23),
                (69, 73, -0.54, 0.02, 0.05),
                (54, 38, -1.05, 0.36, -0.44),
                (56, 39, -0.82, 0.32, 0.18),
                (55, 21, -0.44, 0.12, -0.48),
            ]:
                if not marks[left].x < 0 < marks[right].x:
                    raise RuntimeError("Unexpected landmark order")
                offsets[left] = u.Vector(dx, dy, dz)
                offsets[right] = u.Vector(-dx, dy, dz)
            offsets[64] = u.Vector(0.04, 0.77, -0.45)  # narrow, projecting tip
            offsets[65] = u.Vector(0, 0.34, -0.19)
            offsets[6] = u.Vector(0, 0.22, 0.09)
            offsets[62] = u.Vector(0, 0.29, -0.49)
            creator.translate_face_landmarks(character, list(offsets), list(offsets.values()))
            creator.commit_face_state(character)
            save(character)
        if not assets.does_asset_exist(ROOT + "/Hermit_Head"):
            params = u.MetaHumanGeometryExportParams()
            params.project_path = ROOT
            params.head_skeletal_mesh = True
            params.body_skeletal_mesh = False
            params.full_body_skeletal_mesh = False
            u.MetaHumanCharacterExportBlueprintLibrary.export_geometry(character, params)
        head = assets.load_asset(ROOT + "/Hermit_Head")
        if not head:
            raise RuntimeError("Creator did not export the sculpted head")
        # Walter's saved skin maps have the same facial UVs as his custom sculpt.
        template = assets.load_asset(ROOT + "/Walter_Head")
        slots = {str(m.material_slot_name): m.material_interface for m in template.materials}
        skin = slots[next(k for k in slots if k.lower().startswith("head"))]
        for n in edit.get_material_expressions(skin):
            if (
                isinstance(n, u.MaterialExpressionCustom)
                and n.get_editor_property("description") == "Natural brows, short stubble and hairline"
            ):
                n.set_editor_property(
                    "code",
                    """
float m = saturate(max(Scalp * .68, max(pow(Brows,.52)*1.45, max(Beard*.36, Lip*.60))));
float3 weather = saturate((Skin.rgb - .125)*1.09 + .125) * float3(1.04,.93,.87);
return lerp(weather,float3(.055,.042,.034),m);
""",
                )
        compile_with_renderer(skin)
        save(skin)
        updated = list(head.materials)
        for slot in updated:
            slot.material_interface = slots[str(slot.material_slot_name)]
        head.materials = updated
        save(head)
        jog = duplicate("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd", "A_HermitJog")
        jog.set_editor_properties(
            {
                "force_root_lock": True,
                "enable_root_motion": False,
                "root_motion_root_lock": u.RootMotionRootLock.REF_POSE,
            }
        )
        save(jog)
    finally:
        creator.remove_object_to_edit(character)


class SurfaceMesh:
    def __init__(self):
        self.v, self.f, self.uv, self.colors = [], [], [], []

    def grid(
        self, rows, cols, point, uv=lambda t, s: (s, t), color=lambda t, s: (1, 1, 1), reverse=False
    ):
        first = len(self.v)
        for i in range(rows + 1):
            t = i / rows
            for j in range(cols + 1):
                s = j / cols
                self.v.append(u.Vector(*point(t, s)))
                self.uv.append(u.Vector2D(*uv(t, s)))
                self.colors.append(u.LinearColor(*color(t, s), 1))
                if not i or not j:
                    continue
                d = first + i * (cols + 1) + j
                a, b, c = d - cols - 2, d - cols - 1, d - 1
                pair = [(a, b, d), (a, d, c)]
                self.f.extend(u.IntVector(*(tri[::-1] if reverse else tri)) for tri in pair)

    def save(self, name, material, head, folder=ROOT):
        m = u.DynamicMesh()
        buffers = u.GeometryScriptSimpleMeshBuffers(
            vertices=self.v, triangles=self.f, uv0=self.uv, vertex_colors=self.colors
        )
        m.append_buffers_to_mesh(buffers)
        m.set_per_vertex_normals()
        m.inverse_transform_mesh(head)
        path = folder + "/" + name
        if assets.does_asset_exist(path):
            target = assets.load_asset(path)
            _, result = u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
                m, target, u.GeometryScriptCopyMeshToAssetOptions(), u.GeometryScriptMeshWriteLOD()
            )
        else:
            opts = u.GeometryScriptCreateNewStaticMeshAssetOptions()
            opts.set_editor_properties(dict(enable_collision=False, enable_recompute_tangents=True))
            target, result = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(
                m, path, opts
            )
        if result != u.GeometryScriptOutcomePins.SUCCESS:
            raise RuntimeError("Failed to write " + name)
        target.set_material(0, material)
        save(target)


def authored_material(name, hair=False):
    mat = (
        assets.load_asset(ROOT + "/" + name)
        if assets.does_asset_exist(ROOT + "/" + name)
        else tools.create_asset(name, ROOT, u.Material, u.MaterialFactoryNew())
    )
    existing = list(edit.get_material_expressions(mat))
    mat.set_editor_properties(
        dict(two_sided=True, blend_mode=u.BlendMode.BLEND_MASKED if hair else u.BlendMode.BLEND_OPAQUE)
    )

    def node(cl):
        for i, expression in enumerate(existing):
            if isinstance(expression, cl):
                return existing.pop(i)
        return edit.create_material_expression(mat, cl)

    def constant(value, pin):
        n = node(u.MaterialExpressionConstant)
        n.set_editor_property("r", value)
        edit.connect_material_property(n, "", pin)

    v = node(u.MaterialExpressionVertexColor)
    uv = node(u.MaterialExpressionTextureCoordinate)
    shade = node(u.MaterialExpressionCustom)
    sources = [("V", v, ""), ("UV", uv, "")]
    if hair:
        tex = assets.load_asset(ROOT + "/T_WhiskerStrands")
        a = node(u.MaterialExpressionTextureSample)
        a.set_editor_properties(dict(texture=tex, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS))
        sources.append(("A", a, "RGB"))
        code = """float w=saturate(V.r+.27*(A.g-.50)+.11*A.b);
float3 salt=lerp(float3(.035,.026,.020),float3(.57,.54,.47),w*w);
float3 pepper=lerp(float3(.015,.012,.010),float3(.19,.168,.134),w);
return lerp(salt,pepper,V.g) * lerp(.85,1.06,A.b);"""
        edit.connect_material_property(a, "R", u.MaterialProperty.MP_OPACITY_MASK)
        mat.set_editor_property("opacity_mask_clip_value", 0.28)
    else:
        code = """float2 grain=UV*float2(900,470); float2 filtered=saturate(1.-fwidth(grain));
float felt=.035*dot(filtered,sin(grain*6.28));
float bloom=.06*sin(37*UV.x+7*sin(24*UV.y))*.8*cos(41*UV.y-5*sin(16*UV.x));
return V.rgb*(1.+felt+bloom);"""
    pins = []
    for n, src, channel in sources:
        pin = u.CustomInput()
        pin.set_editor_property("input_name", n)
        pins.append(pin)
    shade.set_editor_properties(
        dict(
            description=name, inputs=pins, code=code, output_type=u.CustomMaterialOutputType.CMOT_FLOAT3
        )
    )
    for n, src, channel in sources:
        if not edit.connect_material_expressions(src, channel, shade, n):
            raise RuntimeError("Cannot wire " + n)
    edit.connect_material_property(shade, "", u.MaterialProperty.MP_BASE_COLOR)
    constant(0.73 if hair else 0.86, u.MaterialProperty.MP_ROUGHNESS)
    constant(0.27, u.MaterialProperty.MP_SPECULAR)
    compile_with_renderer(mat)
    save(mat)
    return mat


def felt_hat(bind):
    brow = bind.get_bone_transform("FACIAL_C_Forehead", True).translation
    h = SurfaceMesh()
    cx, cy, cz = brow.x, brow.y - 9.3, brow.z + 0.95

    def tint(t, s):
        a = 2 * pi * s
        fade = 0.17 * sin(4 * a + 0.4) * cos(1.8 * t) + 0.10 * sin(9 * a + 2 * t) * sin(pi * t)
        return tuple(v * (1 + fade) for v in (0.167, 0.142, 0.096))

    def crown(t, s, extra=0):
        a = 2 * pi * s
        side, front = cos(a), sin(a)
        pinch = (
            exp(-(((abs(side) - 0.48) / 0.29) ** 2)) * max(0, front) * exp(-(((t - 0.76) / 0.26) ** 2))
        )
        radius = 1 - 0.13 * t - 0.055 * t**3 - 0.17 * pinch
        x = (8.95 * radius + extra) * side + cx - 0.93 * t
        y = (10.55 * radius + extra) * front + cy - 0.53 * t
        top = 11.55 + 1.96 * abs(side) ** 1.2 - 1.25 * max(0.0, front)
        z = cz + t * top - 0.28 * side + 0.22 * front
        return x, y, z

    h.grid(23, 112, crown, color=tint)

    # Shallow longitudinal crease across the pinched crown.
    def top(t, s):
        a = 2 * pi * s
        x, y, z = crown(1, s)
        k = (1 - t) * 0.998 + 0.002
        px = (x - cx + 0.93) * k
        py = (y - cy + 0.53) * k
        return (
            cx - 0.93 + px,
            cy - 0.53 + py,
            cz
            + 11.90
            + 2.25 * (abs(px) / 8) ** 1.35
            - 0.67 * (1 - k)
            + 0.37 * sin(a * 2) * k
            - 0.92 * max(0.0, sin(a)) * k,
        )

    h.grid(15, 112, top, color=tint)

    def brim(t, s):
        a = 2 * pi * s
        side, front = cos(a), sin(a)
        flare = 1 + 0.07 * sin(a - 0.2)
        x = cx + side * (8.83 + t * (6.25 * flare)) + 0.22 * t * sin(3 * a)
        y = cy + front * (10.40 + t * (5.4 * flare))
        lifted = (
            2 * abs(side) ** 2.5
            + 1.24 * abs(side) * side
            - 1.8 * max(0, front) ** 2
            + 0.85 * sin(3 * a - 0.5)
            + 0.70
        )
        z = cz - 0.38 + 0.19 * sin(a - 0.7) - 0.40 * side + t * (-1.05 + lifted * t)
        return x, y, z

    h.grid(18, 128, brim, color=tint)
    # Bound edge and underside show in silhouette when his head is tipped down.
    h.grid(
        2,
        128,
        lambda t, s: tuple(v - 0.30 * t if i == 2 else v for i, v in enumerate(brim(1, s))),
        color=lambda t, s: (0.086, 0.074, 0.054),
    )
    h.grid(
        9,
        128,
        lambda t, s: tuple(v - 0.30 if i == 2 else v for i, v in enumerate(brim(t, s))),
        color=lambda t, s: (0.093, 0.082, 0.062),
        reverse=True,
    )
    # Grosgrain band and its knot sit around the crown instead of intersecting the brim.
    h.grid(
        6,
        112,
        lambda t, s: crown(0.045 + 0.237 * t, s, 0.08 + 0.07 * sin(pi * t)),
        color=lambda t, s: (0.044, 0.037, 0.029),
    )
    for edge in [0.046, 0.273]:
        h.grid(
            3,
            112,
            lambda t, s: crown(edge + 0.011 * t, s, 0.17 + 0.04 * sin(pi * t)),
            color=lambda t, s: (0.064, 0.054, 0.041),
        )

    def bow(t, s):
        a = pi + (s - 0.5) * 0.64
        z = 0.073 + 0.17 * (t - 0.08) * (0.47 + abs(s - 0.5))
        return crown(z, a / (2 * pi), 0.20 + 0.28 * abs(s - 0.5))

    h.grid(5, 22, bow, color=lambda t, s: (0.054, 0.044, 0.031))
    # Tip the felt off his eyes. The high, pinched front remains visible in both the
    # upright walk and the forward-leaning shoulder pose.
    tilt = 12 * pi / 180
    for index, vertex in enumerate(h.v):
        dy, dz = vertex.y - cy, vertex.z - cz
        h.v[index] = u.Vector(
            vertex.x,
            cy - 0.75 + dy * cos(tilt) - dz * sin(tilt),
            cz + 0.75 + dy * sin(tilt) + dz * cos(tilt),
        )
    h.save("SM_HermitFeltHat", authored_material("M_HermitFelt"), bind.get_bone_transform("head", True))


def whiskers(bind):
    from pathlib import Path

    source = (
        Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
        / "Art/Source/FirstLeg/Hermit/T_WhiskerStrands.png"
    )
    task = u.AssetImportTask()
    task.set_editor_properties(
        dict(
            filename=str(source), destination_path=ROOT, automated=True, replace_existing=True, save=True
        )
    )
    tools.import_asset_tasks([task])
    tex = assets.load_asset(ROOT + "/T_WhiskerStrands")
    tex.set_editor_properties(
        dict(srgb=False, compression_settings=u.TextureCompressionSettings.TC_MASKS)
    )
    save(tex)
    fur = authored_material("M_HermitFacialHair", True)
    cards = SurfaceMesh()
    rng = Random(1918)
    chin = bind.get_bone_transform("FACIAL_C_Chin", True).translation
    lip = bind.get_bone_transform("FACIAL_C_LipUpper", True).translation

    def card(point, side, width, white, pepper, col, steps=12):

        def surface(t, s):
            p = point(t)
            across = side(t)
            spread = width * (0.08 + 0.92 * min(1.0, t * 8)) * (1 - 0.92 * t**3) * (s - 0.5)
            return tuple(p[i] + across[i] * spread for i in range(3))

        cards.grid(
            steps,
            2,
            surface,
            lambda t, s: ((col * 128 + 9 + 110 * s) / 1024, (14 + 480 * t) / 512),
            lambda t, s: (white * (0.49 + 0.51 * min(1.0, t * 3.8)), pepper, t),
        )

    # Start each lock on the surface of the edited head, including the whiskers that
    # grow nearly horizontally across the cheeks before falling into the long beard.
    skin = u.DynamicMesh()
    _, ok = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        assets.load_asset(ROOT + "/Hermit_Head"),
        skin,
        u.GeometryScriptCopyMeshFromAssetOptions(),
        u.GeometryScriptMeshReadLOD(),
    )
    if ok != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Could not read the sculpt to plant facial hair")
    bvh = skin.build_bvh_for_mesh()[-1]
    query = u.GeometryScriptSpatialQueryOptions()

    def skin_at(x, y, z, nx, ny, fallback=None):
        hit = skin.find_nearest_ray_intersection_with_mesh(
            bvh, u.Vector(x + 30 * nx, y + 30 * ny, z), u.Vector(-nx, -ny, 0), query
        )[-2]
        if not hit.hit:
            if fallback is not None:
                return fallback
            raise RuntimeError("Cannot find skin at the whisker root")
        v = hit.hit_position
        return (v.x - 0.15 * nx, v.y - 0.15 * ny, z)

    def strand(a, outer, separation, short=False):
        sn, cs = sin(a), cos(a)
        cheek = abs(sn) ** 1.65
        z = chin.z + 0.72 + 4.46 * cheek + rng.uniform(-3.6, 0.65)
        if short:
            z += rng.uniform(0.11, 0.70)
        start = skin_at(chin.x, lip.y - 10.45, z, sn, cs)
        angle = rng.uniform(-0.18, 0.18)
        phase = rng.uniform(0, 2 * pi)
        curl = rng.uniform(0.16, 0.66)
        tuck = sin(abs(a)) * 0.32
        if short:
            length = rng.uniform(1.9, 4.8)
        else:
            length = separation + z - (chin.z - 16.9) + rng.uniform(-2.4, 1.9) + 0.4 * cos(8 * a)
        length = max(1.0, length)

        def path(t):
            flow = a * (1 - (0.18 + tuck * 0.25) * t) + angle * t + 0.042 * sin(phase + 7 * t) * t
            endx = chin.x + sin(flow) * (6.4 if not short else 7.1)
            endy = lip.y - 9.4 + cos(flow) * (7.5 if not short else 9.0)
            reach = min(1.0, t * length / 14)
            lift = (1.2 + 1.3 * outer) * sin(min(1.0, t * length / 9) * pi * 0.75)
            # Small differences in curl and the longer crossing side locks break the
            # parallel silhouette while maintaining the natural direction of growth.
            return (
                start[0] * (1 - reach) + endx * reach + sn * lift + cs * curl * t * sin(phase + 8 * t),
                start[1] * (1 - reach) + endy * reach + cs * lift - sn * curl * t * sin(phase + 8 * t),
                z - length * t,
            )

        pigment = max(
            0.12,
            min(0.96, 0.48 + 0.21 * (1 - cheek) + 0.12 * cos(12 * a + 0.4) + rng.uniform(-0.23, 0.27)),
        )
        return path, pigment, (cs, -sn, 0)

    for j in range(4150):
        a = rng.uniform(-1.76, 1.76)
        outer = rng.random()
        separation = rng.uniform(-1.9, 2.05)
        fine = j >= 1500
        short = j >= 3650
        path, pigment, across = strand(a, outer, separation, short)
        width = rng.uniform(0.055, 0.152) if fine else rng.uniform(0.24, 0.54)
        col = rng.randrange(6, 8) if fine else rng.randrange(0, 6)
        card(
            path,
            lambda t: across,
            width,
            pigment,
            0.08 if not short else 0.24,
            col,
            11 if not short else 6,
        )
    # The moustache begins at actual skin under the nose. Individual hairs cross the
    # philtrum and sweep outwards, gradually joining the longer hairs at the corners.
    for sign in (-1, 1):
        for j in range(510):
            x = rng.uniform(-0.14, 3.05)
            q = min(1, max(0, x / 3.05))
            z = lip.z + 0.33 + 0.53 * (1 - q) + rng.uniform(-0.24, 0.32)
            pt = skin_at(lip.x + sign * x, lip.y, z, 0, 1)
            length = rng.uniform(1.30, 3.4) * (1 - 0.36 * q)
            phase = rng.uniform(0, 2 * pi)
            fall = 0.55 + 1.45 * q + rng.uniform(0.0, 0.7)

            def path(t):
                return (
                    pt[0] + sign * (length * t + 0.11 * t * sin(phase + 5 * t)),
                    pt[1] + 0.64 * sin(t * pi * 0.85) - 0.94 * q * t,
                    pt[2] - fall * t**1.5 + 0.11 * sin(4.4 * t + phase) * t,
                )

            card(
                path,
                lambda t: (0, 0.13, -1),
                rng.uniform(0.08, 0.29),
                rng.uniform(0.22, 0.70),
                0.79,
                rng.randrange(4, 8),
                8,
            )
    # Short whiskers curl over the chin and out from the corners of the mouth. They
    # link the dense moustache to the hanging beard, leaving the lip line visible.
    for j in range(2400):
        x = rng.uniform(-4.65, 4.65)
        edge = abs(x) / 4.65
        top = lip.z - 1.18 + 1.62 * edge**1.55
        z = top - rng.uniform(0, 2.75)
        sx, sy, sz = skin_at(lip.x + x, lip.y, z, 0, 1)
        length = rng.uniform(1.8, 5.9) + 0.48 * edge
        end_x = lip.x + x + (0.16 + 0.53 * edge) * (1 if x > 0 else -1) + rng.uniform(-0.61, 0.61)
        ex, ey, ez = skin_at(end_x, lip.y, z - length, 0, 1, (end_x, sy - 0.38 * length, z - length))
        # Spring away from the labiomental crease into the fuller white chin.
        loft = rng.uniform(0.45, 1.05)
        curl = rng.uniform(-0.28, 0.28)

        def lower(t):
            return (
                sx + (ex - sx) * t + curl * sin(5 * t) * t,
                sy + (ey - sy) * t + loft * sin(t * pi * 0.84),
                sz + (ez - sz) * t,
            )

        white = rng.uniform(0.32, 0.88) + 0.08 * (1 - edge)
        card(
            lower,
            lambda t: (1, 0, 0),
            rng.uniform(0.085, 0.31),
            white,
            0.10 + 0.18 * edge,
            rng.randrange(2, 8),
            7,
        )
    # Wavy hair around the nape, with short flyaways near the ear and longer white tips.
    for j in range(680):
        a = rng.uniform(1.35, 2 * pi - 1.35)
        sn, cs = sin(a), cos(a)
        phase = rng.uniform(0, 2 * pi)
        z = chin.z + 14.1 + rng.uniform(-1.4, 1.5)
        length = rng.uniform(7.1, 13.1)

        def path(t):
            return (
                chin.x + sn * (7.6 + 0.73 * sin(t * pi)) + 0.51 * t * sin(phase + 7 * t),
                lip.y - 11.1 + cs * (8.8 + t) - 0.55 * t,
                z - length * t,
            )

        card(
            path,
            lambda t: (cs, -sn, 0),
            rng.uniform(0.16, 0.49),
            rng.uniform(0.42, 0.88),
            0.15,
            rng.randrange(8),
            10,
        )
    # Eyebrows use the baked Creator skin/follicle mask, which follows the eyelids.
    cards.save("SM_HermitWhiskers", fur, bind.get_bone_transform("head", True))


def build_accessories():
    bind = u.SkeletonModifier()
    if not bind.set_skeletal_mesh(u.load_asset(ROOT + "/Hermit_Head")):
        raise RuntimeError("Build the sculpted face before fitting headwear.")
    felt_hat(bind)
    whiskers(bind)
