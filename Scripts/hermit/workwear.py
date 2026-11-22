"""Tailor a checked collar shirt, broad bib overalls and worn boots to the animated body."""

from math import cos, pi, sin

import create_max_outfit as sew
import unreal as u
from hermit.appearance import (
    MAX,
    ROOT,
    assets,
    compile_with_renderer,
    duplicate,
    edit,
    save,
    tools,
)

SWATCHES = {
    "Shirting": ("CheckedCotton", (0.59, 0.55, 0.44), 0.86, 0.85),
    "WashedDenim": ("WornIndigo", (0.064, 0.096, 0.118), 0.88, 1.3),
    "Seam": ("TobaccoThread", (0.205, 0.147, 0.080), 0.89, 0.6),
    "Brass": ("OldBrass", (0.29, 0.184, 0.072), 0.42, 0.08),
    "OxbloodCanvas": ("BootLeather", (0.079, 0.045, 0.027), 0.67, 0.27),
    "Rubber": ("BootSoles", (0.033, 0.030, 0.025), 0.82, 0.13),
    "Laces": ("BootLaces", (0.116, 0.089, 0.055), 0.83, 0.50),
}


def fabrics():
    base = u.load_asset(MAX + "/Wardrobe/M_TrailFabric")
    checked = duplicate(base.get_path_name(), "M_HermitCheckedShirt")
    for node in edit.get_material_expressions(checked):
        if (
            isinstance(node, u.MaterialExpressionCustom)
            and str(node.get_editor_property("description")) == "Dyed cloth with filtered warp and weft"
        ):
            node.set_editor_property(
                "code",
                """
float2 p=UV*float2(14,8); float2 aa=fwidth(p);
float2 lines=1.-smoothstep(.04-aa*.4,.088+aa*.4,abs(frac(p+.5)-.5));
float2 second=1.-smoothstep(.00,.018+aa*.4,abs(frac(p+.43)-.5));
float2 filter=saturate(1.-aa*2.3); float check=dot(lines*filter,float2(.53,.46));
float slub=.038*sin(dot(UV,float2(83,213)))*cos(dot(UV,float2(165,-67)));
return Tint.rgb * (1.0 + Weave*slub - check) - float3(.035,.039,.033)*dot(second*filter,float2(.65,.49));""",
            )
    compile_with_renderer(checked)
    save(checked)
    slots = {}
    for key, (name, rgb, rough, weave) in SWATCHES.items():
        path = ROOT + "/MI_" + name
        mi = (
            assets.load_asset(path)
            if assets.does_asset_exist(path)
            else tools.create_asset(
                "MI_" + name, ROOT, u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew()
            )
        )
        edit.set_material_instance_parent(mi, checked if key == "Shirting" else base)
        edit.set_material_instance_vector_parameter_value(mi, "Tint", u.LinearColor(*rgb, 1))
        edit.set_material_instance_scalar_parameter_value(mi, "Roughness", rough)
        edit.set_material_instance_scalar_parameter_value(mi, "Weave", weave)
        save(mi)
        slots[key] = mi
    return slots


class WorkPanels(sew.Panels):
    def __init__(self):
        self.by_mat = {key: [[], [], [], {}] for key in SWATCHES}

    def patched_sleeve(self, surface, _):
        self.grid(35, 52, surface, "Shirting", True)

    def cord(self, path, r, material):
        if material == "OchreYoke":
            return  # Plain shirt elbows.
        replacements = {"IndigoShell": "Shirting", "DarkBinding": "Rubber"}
        super().cord(path, r, replacements.get(material, material))


def shirt_and_bib(tailor):
    s, p, chest, shoulder, neck = tailor.s, tailor.panels, tailor.chest, tailor.shoulder, tailor.neck
    skirt, waist = tailor.hip - 1.7 * s, tailor.hip + 7 * s

    def shirt(h, a):
        if h <= 0.65:
            z = sew.mix(skirt, chest, h / 0.65)
            ease = (1.95 + 0.32 * sin(5 * a + 0.075 * z) + 0.15 * cos(9 * a - 0.048 * z)) * s
            return tailor.torso(z, a, ease)
        ridge = (
            sin(a) * (abs(shoulder[0]) + 2.9 * s),
            neck[1] + cos(a) * (9.3 + max(0, cos(a))) * s,
            shoulder[2] + (3.65 - 4.2 * max(0, cos(a)) ** 2 - 0.55 * abs(cos(a))) * s,
        )
        if h <= 0.83:
            return sew.lerp(shirt(0.65, a), ridge, (h - 0.65) / 0.18)
        collar = (
            5.6 * s * sin(a),
            neck[1] + 5.3 * s * cos(a),
            neck[2] + s * (0.65 - 2.2 * max(0, cos(a))),
        )
        return sew.lerp(ridge, collar, (h - 0.83) / 0.17)

    waist_h = (waist - skirt) / (chest - skirt) * 0.65
    p.grid(44, 80, lambda t, j: shirt(sew.mix(waist_h - 0.04, 1.0, t), j * 2 * pi), "Shirting")

    def cover(h, a, thickness):
        return sew.lift(shirt(h, a), sew.circular(a), thickness * s)

    # Two folded collar points run from the back neck to either side of the shirt opening.
    for side in (-1, 1):

        def collar(t, j):
            a = side * (0.22 + j * (pi - 0.22))
            tip = sew.bell(j, 0.095, 0.12)
            return cover(sew.mix(1.0, 0.945 - 0.075 * tip, t), a, 0.42 + 0.38 * t)

        p.grid(9, 40, collar, "Shirting", side < 0)
    p.cord([cover(0.72 + 0.263 * k / 20, 0, 0.31) for k in range(21)], 0.36 * s, "Shirting")
    for h in (0.80, 0.875, 0.950):
        x, y, z = cover(h, 0, 0.65)
        p.cord(
            [
                (x + 0.25 * s * sin(2 * pi * k / 14), y, z + 0.25 * s * cos(2 * pi * k / 14))
                for k in range(15)
            ],
            0.11 * s,
            "Seam",
        )

    def bib(t, j, back=False):
        height = sew.mix(waist_h, 0.696 if not back else 0.59, t)
        span = sew.mix(1.31, 0.695 if not back else 0.55, t)
        a = (j - 0.5) * 2 * span + (pi if back else 0)
        return cover(height, a, 0.78 + 0.24 * sin(pi * t) * cos(a * 3))

    for back in (False, True):
        p.grid(26, 35, lambda t, j: bib(t, j, back), "WashedDenim")
        for side in (0.0, 1.0):
            p.cord([bib(k / 30, side, back) for k in range(31)], 0.13 * s, "Seam")
        p.cord([bib(1, j / 36, back) for j in range(37)], 0.19 * s, "Seam")
    p.grid(
        13,
        88,
        lambda t, j: cover(waist_h + (t - 1) * 0.225, j * 2 * pi, sew.mix(-2.0, 0.85, t)),
        "WashedDenim",
    )
    p.cord([cover(waist_h - 0.017, j / 90 * 2 * pi, 0.94) for j in range(91)], 0.12 * s, "Seam")
    # Denim braces travel over the shoulders and meet the back bib. The player sees the
    # shoulder buttons and adjusters as the resident turns toward Max.
    for sign in (-1, 1):

        def brace(t, j):
            width = (j - 0.5) * 0.336
            if t <= 0.44:
                k = t / 0.44
                h = sew.mix(0.665, 0.856, k)
                a = sew.mix(0.53, 0.95, k) + width
            elif t <= 0.64:
                k = (t - 0.44) / 0.20
                h = 0.856 + 0.019 * sin(pi * k)
                a = sew.mix(0.95, pi - 0.92, k) + width
            else:
                k = (t - 0.64) / 0.36
                h = sew.mix(0.856, 0.568, k)
                a = sew.mix(pi - 0.92, pi - 0.43, k) + width
            return cover(h, sign * a, 1.08 + 0.24 * sin(t * pi))

        p.grid(44, 9, brace, "WashedDenim", sign < 0)
        for edge in (0.06, 0.94):
            p.cord([brace(i / 50, edge) for i in range(51)], 0.105 * s, "Seam")
        for h in (0.071, 0.136):
            p.cord([brace(h, j / 12) for j in range(13)], 0.25 * s, "Brass")
        for j in (0.0, 1.0):
            p.cord([brace(0.071 + 0.065 * i / 5, j) for i in range(6)], 0.19 * s, "Brass")
        x, y, z = brace(0.011, 0.50)
        p.cord(
            [
                (x + 0.55 * s * cos(i * pi / 9), y + 0.19 * s, z + 0.55 * s * sin(i * pi / 9))
                for i in range(19)
            ],
            0.25 * s,
            "Brass",
        )

    def pocket(t, j):
        a = (j - 0.5) * 0.77
        start = 0.43 + 0.049 * abs(j - 0.5) * 2
        return cover(sew.mix(start, 0.593, t), a, 1.56 + 0.30 * sin(pi * t) * sin(pi * j))

    p.grid(18, 24, pocket, "WashedDenim")
    for t in (0.0, 0.93, 1.0):
        p.cord([pocket(t, j / 28) for j in range(29)], 0.13 * s, "Seam")
    for j in (0.0, 1.0):
        p.cord([pocket(i / 20, j) for i in range(21)], 0.12 * s, "Seam")


def build_outfit():
    body = u.load_asset(MAX + "/Anatomy/Max_Body")
    mesh = u.DynamicMesh()
    _, outcome = u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
        body, mesh, u.GeometryScriptCopyMeshFromAssetOptions(), u.GeometryScriptMeshReadLOD()
    )
    if outcome != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError("Could not open Creator body for tailoring")
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
    tailor.panels = WorkPanels()
    shirt_and_bib(tailor)
    tailor.pants(pos, tri)
    for side in ("l", "r"):
        tailor.sleeve(side)
        tailor.shoe(side, pos)
    cloth = tailor.panels.dynamic()
    mesh.transfer_bone_weights_from_mesh(cloth)
    for vertex in range(0, cloth.get_vertex_count(), 41):
        weights, valid = cloth.get_vertex_bone_weights(vertex)[-2:]
        if not valid or not weights or abs(sum(w.weight for w in weights) - 1) > 0.01:
            raise RuntimeError("Unweighted workwear vertex " + str(vertex))
    sew.save_mesh(cloth, "SK_HermitWorkwear", body.skeleton, fabrics(), ROOT)
