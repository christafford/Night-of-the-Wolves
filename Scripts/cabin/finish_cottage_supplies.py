"""Fit tactile rope and cloth within the original pickup bounds and bounce the hearth light onto them."""

from pathlib import Path
import math
import sys
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_log_cabin import bake, assets, edit, tools, DEST
from carpentry_mesh import Joinery, V, TAU

ITEMS = "/Game/NightOfTheWolves/FirstLeg"


def fibres(name, low, light, weave=False):
    path = f"{DEST}/{name}"
    if assets.does_asset_exist(path):
        m = assets.load_asset(path)
        edit.delete_all_material_expressions(m)
    else:
        m = tools.create_asset(name, DEST, u.Material, u.MaterialFactoryNew())
    m.set_editor_property("used_with_nanite", True)

    def expr(kind, x, y, **props):
        e = edit.create_material_expression(m, kind, x, y)
        if props:
            e.set_editor_properties(props)
        return e

    custom = expr(u.MaterialExpressionCustom, -360, 0)
    uv = expr(u.MaterialExpressionTextureCoordinate, -640, 0)
    cfg = u.CustomInput()
    cfg.set_editor_property("input_name", "UV")
    if weave:
        detail = """
float2 p=UV*float2(115,63);
float h=.5+.5*cos(p.x*6.2832+.38*sin(p.y*2));
float w=.5+.5*cos(p.y*6.2832+.28*sin(p.x*1.7));
float over=fmod(floor(p.x)+floor(p.y),2);
float thread=lerp(h*.75+w*.25,w*.75+h*.25,over);
float fleck=sin(dot(p,float2(.37,1.62)))*.09+cos(p.y*.71)*.07;
float wear=saturate(.15+.64*thread+fleck);
"""
    else:
        detail = """
float filament=.5+.5*cos(6.2832*(UV.y*11.0+UV.x*28)+.8*sin(UV.x*12));
float fleck=.06*sin(UV.x*147+UV.y*13);
float wear=saturate(.24+.53*filament+fleck);
"""
    fmt = lambda c: ",".join(map(str, c))
    custom.set_editor_properties(
        dict(
            inputs=[cfg],
            output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
            code=detail + f"return lerp(float3({fmt(low)}),float3({fmt(light)}),wear);",
        )
    )
    edit.connect_material_expressions(uv, "", custom, "UV")
    edit.connect_material_property(custom, "", u.MaterialProperty.MP_BASE_COLOR)
    rough = expr(u.MaterialExpressionConstant, -360, 180, r=0.87)
    edit.connect_material_property(rough, "", u.MaterialProperty.MP_ROUGHNESS)
    assets.save_loaded_asset(m, False)
    return m


def tube(s, slot, path, radius, closed=False, sides=7):
    rows = []
    dist = [0.0]
    for a, b in zip(path, path[1:]):
        dist.append(dist[-1] + (a - b).length())
    for i, p in enumerate(path):
        prev = path[i - 1] if closed or i else p
        after = path[(i + 1) % len(path)] if closed or i + 1 < len(path) else p
        tangent = (after - prev).normal()
        along = tangent.cross(
            V(0, 1, 0) if abs(tangent.y) < 0.91 else V(1, 0, 0)
        ).normal()
        up = tangent.cross(along).normal()
        directions = [
            along * math.cos(TAU * j / sides) + up * math.sin(TAU * j / sides)
            for j in range(sides)
        ]
        rows.append([(p + d * radius, d) for d in directions])
    total = dist[-1] + ((path[-1] - path[0]).length() if closed else 0)
    for i in range(len(path) if closed else len(path) - 1):
        k = (i + 1) % len(path)
        for j in range(sides):
            h = (j + 1) % sides
            points, normals = zip(rows[i][j], rows[k][j], rows[k][h], rows[i][h])
            u0, u1 = dist[i] / 7, (total if k == 0 else dist[k]) / 7
            s.face(
                slot,
                points,
                [
                    (u0, j / sides),
                    (u1, j / sides),
                    (u1, (j + 1) / sides),
                    (u0, (j + 1) / sides),
                ],
                (normals[0] + normals[-1]).normal(),
                1,
                normals,
            )
    if not closed:
        for i, direction in ((0, path[0] - path[1]), (-1, path[-1] - path[-2])):
            s.face(
                slot,
                [a for a, b in rows[i]],
                [
                    (
                        0.5 + 0.48 * math.cos(TAU * j / sides),
                        0.5 + 0.48 * math.sin(TAU * j / sides),
                    )
                    for j in range(sides)
                ],
                direction.normal(),
                1,
            )


def coil():
    s = Joinery()
    for loop in range(5):
        for strand in range(3):
            path = []
            for i in range(153):
                a = TAU * i / 153
                center = V(
                    9.7 * math.cos(a),
                    1.44 * loop + 0.52 * math.sin(2 * a + loop),
                    14.8 * math.sin(a) + 0.28 * math.sin(3 * a + loop),
                )
                radial = V(math.cos(a), 0, math.sin(a)).normal()
                twist = 15 * a + TAU * strand / 3
                path.append(
                    center
                    + radial * (0.63 * math.cos(twist))
                    + V(0, 0.63 * math.sin(twist), 0)
                )
            tube(s, 0, path, 0.54, True, 6)
    # Three binding turns compress the bundle near its head, just below the peg.
    for i in range(3):
        path = [
            V(
                -6.4 + i * 0.67 + 0.2 * math.sin(TAU * j / 25),
                2.85 + 4.53 * math.cos(TAU * j / 25),
                11.9 + i * 0.36 + 1.94 * math.sin(TAU * j / 25),
            )
            for j in range(25)
        ]
        tube(s, 0, path, 0.48, True)
    tube(
        s,
        0,
        [
            V(-6, -1.2, 12),
            V(-7, -1.6, 9),
            V(-8.1, -1.2, 5.2),
            V(-8, -1.8, 3.2),
            V(-8.5, -1.7, 2.3),
        ],
        0.53,
    )
    return s


def purse():
    s = Joinery()
    profile = [
        (-16, 4.45),
        (-15.6, 6.1),
        (-14.8, 7.4),
        (-12.1, 8.5),
        (-9.3, 8.9),
        (-6.8, 8.7),
        (-4.8, 7.5),
        (-3, 5.9),
        (-1, 3.8),
        (0.65, 3),
        (1.30, 3.15),
        (2.24, 3.65),
        (3.53, 4.8),
        (3.95, 4.96),
    ]
    count = 44
    rings = []
    for z, r in profile:
        row = []
        for j in range(count):
            a = TAU * j / count
            lobes = (
                1
                + 0.05 * math.cos(5 * a + 0.27 * z)
                + 0.075 * math.sin(7 * a - 0.16 * z)
                + 0.034 * math.cos(12 * a + 0.12 * z)
            )
            puckering = 1 if z < -0.5 else 1 + 0.07 * math.sin(9 * a)
            x = r * math.cos(a) * lobes * puckering
            y = 0.84 * r * math.sin(a) * lobes * puckering
            zz = z + (0.55 * math.cos(a * 7) if z > 2 else 0.1 * math.sin(a * 3))
            row.append(V(x, y, zz))
        rings.append(row)
    for i in range(len(rings) - 1):
        slope = (profile[i + 1][1] - profile[i][1]) / (
            profile[i + 1][0] - profile[i][0]
        )
        for j in range(count):
            k = (j + 1) % count
            normals = [
                V(math.cos(TAU * n / count), math.sin(TAU * n / count), -slope).normal()
                for n in (j, j, k, k)
            ]
            pts = [rings[i][j], rings[i + 1][j], rings[i + 1][k], rings[i][k]]
            zs = [(p.z + 16) / 20 for p in pts]
            s.face(
                0,
                pts,
                [(n / count, z) for n, z in zip((j, j, j + 1, j + 1), zs)],
                normals[0],
                1,
                normals,
            )
    # A rolled cloth edge and dark concave throat prevent a flat, sealed top.
    tube(s, 0, rings[-1], 0.27, True, 6)
    for j in range(count):
        a = rings[-1][j]
        b = rings[-1][(j + 1) % count]
        bottom = V(0.1, 0, 0.94)
        s.face(
            0,
            [a, b, bottom],
            [(j / count, 1), ((j + 1) / count, 1), (0.5, 0.22)],
            V(0, 0, 1),
            0.62,
        )
    s.face(
        0,
        rings[0],
        [(p.x / 13 + 0.5, p.y / 13 + 0.5) for p in rings[0]],
        V(0, 0, -1),
        1,
    )
    for z in (-0.11, 0.47):
        line = [
            V(
                4.22 * math.cos(TAU * i / 38),
                3.52 * math.sin(TAU * i / 38),
                z + 0.15 * math.sin(2 * TAU * i / 38),
            )
            for i in range(38)
        ]
        tube(s, 1, line, 0.39, True)
    for side in (-1, 1):
        line = [
            V(
                1.65 + side * 0.98 * math.cos(TAU * i / 20),
                -3.1 - 0.72 * math.sin(TAU * i / 20),
                0.17 + side * 0.65 * math.sin(TAU * i / 20),
            )
            for i in range(20)
        ]
        tube(s, 1, line, 0.42, True)
        tube(
            s,
            1,
            [
                V(1.65, -3.4, 0.05),
                V(2.2 + side * 0.8, -4.0, -0.8),
                V(2.2 + side * 1.1, -5.5, -2.8),
                V(2.1 + side * 1.25, -6.4, -4.1),
                V(2.1 + side * 0.9, -6.65, -4.9),
            ],
            0.33,
        )
    return s


def bounce_lights():
    lv = u.get_editor_subsystem(u.LevelEditorSubsystem)
    if not lv.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
        raise RuntimeError("Missing trail map")
    group = u.get_editor_subsystem(u.EditorActorSubsystem)
    by = {a.get_actor_label(): a for a in group.get_all_level_actors()}
    for label, pos, power, radius, source in (
        ("Hearth bounce - worktable", (2727, -112, 183), 12.5, 342, 58),
        ("Hearth bounce - tool peg", (3035, -85, 198), 12.8, 262, 60),
        ("Hearth bounce - room", (2840, 20, 237), 4.9, 370, 76),
    ):
        a = by.get(label) or group.spawn_actor_from_class(u.PointLight, V(*pos))
        a.modify()
        a.set_actor_label(label)
        a.set_actor_location(V(*pos), False, False)
        c = a.get_component_by_class(u.PointLightComponent)
        c.modify()
        c.set_mobility(u.ComponentMobility.MOVABLE)
        c.set_editor_property("intensity_units", u.LightUnits.CANDELAS)
        c.set_intensity(power)
        c.set_attenuation_radius(radius)
        c.set_light_color(u.LinearColor(1, 0.835, 0.65, 1), False)
        c.set_editor_property("source_radius", source)
        c.set_editor_property("cast_shadows", False)
        c.set_editor_property("indirect_lighting_intensity", 0.0)
        c.set_editor_property("volumetric_scattering_intensity", 0.0)
        c.set_editor_property("specular_scale", 0.38)
    if not lv.save_current_level():
        raise RuntimeError("Could not save hearth bounce lights")


if __name__ == "__main__":
    rope = fibres("M_TwistedHemp", (0.17, 0.112, 0.061), (0.48, 0.35, 0.205))
    fabric = fibres("M_LinenPouch", (0.131, 0.064, 0.033), (0.365, 0.204, 0.098), True)
    bake("Rope", coil(), [rope], ITEMS)
    bake("Gold", purse(), [fabric, rope], ITEMS)
    bounce_lights()
    u.log("COTTAGE SUPPLIES SAVED shared hemp, linen and interior bounce")
