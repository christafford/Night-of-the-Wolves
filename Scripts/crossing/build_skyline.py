"""Bake the two logging landings, iron carriage and hand sling in Unreal Editor."""

from math import cos, sin, tau, hypot
from pathlib import Path
import sys
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "cabin"))
from carpentry_mesh import Joinery, V, R, TIMBER, BOARD, IRON
from build_log_cabin import assets, tools, edit, bake
from finish_cottage_supplies import tube

DEST = "/Game/NightOfTheWolves/Crossing/Skyline"
WOOD = "/Game/NightOfTheWolves/FirstLeg/Carpentry"
STEEL, OXIDE, PAINT, HEMP = 5, 6, 7, 8
SPAN = hypot(1410, 490) - 215


def metal(name, dark, light, metallic, roughness):
    path = f"{DEST}/{name}"
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    m = tools.create_asset(name, DEST, u.Material, u.MaterialFactoryNew())

    def node(t, x, y, **props):
        n = edit.create_material_expression(m, t, x, y)
        if props:
            n.set_editor_properties(props)
        return n

    tex = node(u.MaterialExpressionTextureCoordinate, -650, 0)
    shade = node(u.MaterialExpressionCustom, -370, 0)
    uv = u.CustomInput()
    uv.set_editor_property("input_name", "p")

    def rgb(c):
        return "float3(" + ",".join(str(i) for i in c) + ")"

    shade.set_editor_properties(
        dict(
            inputs=[uv],
            output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
            code=f"""float s=sin(p.x*16.7 + 2.2*sin(p.y*11.3))*.17 + cos(p.y*13.7+sin(p.x*8.4))*.11;
float fleck=frac(sin(dot(floor(p*140.),float2(12.9898,78.233)))*43758.5453);
return lerp({rgb(dark)},{rgb(light)},saturate(.49+s+.16*fleck));""",
        )
    )
    edit.connect_material_expressions(tex, "", shade, "p")
    edit.connect_material_property(shade, "", u.MaterialProperty.MP_BASE_COLOR)
    for i, (property_, v) in enumerate(
        (
            (u.MaterialProperty.MP_ROUGHNESS, roughness),
            (u.MaterialProperty.MP_METALLIC, metallic),
        )
    ):
        edit.connect_material_property(
            node(u.MaterialExpressionConstant, -230, 180 + 110 * i, r=v), "", property_
        )
    m.set_editor_property("used_with_instanced_static_meshes", True)
    m.set_editor_property("used_with_nanite", True)
    assets.save_loaded_asset(m, False)
    return m


def shaft(s, start, end, radius, slot=STEEL, sides=16):
    tube(s, slot, [V(*start), V(*end)], radius, False, sides)


def ellipse(s, slot, center, x, y, radius, steps=36):
    center, x, y = V(*center), V(*x), V(*y)
    tube(
        s,
        slot,
        [
            center + x * cos(tau * i / steps) + y * sin(tau * i / steps)
            for i in range(steps)
        ],
        radius,
        True,
        8,
    )


def sheave():
    s = Joinery()
    # Turned circumferential section. Opposed flanges retain the cable in a concave groove.
    profile = [
        (22.4, -6.4),
        (28.65, -6.4),
        (29.4, -5.5),
        (29.4, -3.9),
        (25.6, -2.4),
        (24.75, 0),
        (25.6, 2.4),
        (29.4, 3.9),
        (29.4, 5.5),
        (28.65, 6.4),
        (22.4, 6.4),
    ]
    sectors = 54
    for i in range(sectors):
        for j, (radius, y) in enumerate(profile):
            other, yy = profile[(j + 1) % len(profile)]
            pts = [
                V(cos(tau * k / sectors) * r, v, sin(tau * k / sectors) * r)
                for k, r, v in (
                    (i, radius, y),
                    (i + 1, radius, y),
                    (i + 1, other, yy),
                    (i, other, yy),
                )
            ]
            a = tau * (i + 0.5) / sectors
            normal = V((yy - y) * cos(a), radius - other, (yy - y) * sin(a)).normal()
            s.face(
                STEEL if 1 <= j <= 8 else OXIDE,
                pts,
                [
                    (i / sectors, j * 0.13),
                    ((i + 1) / sectors, j * 0.13),
                    ((i + 1) / sectors, (j + 1) * 0.13),
                    (i / sectors, (j + 1) * 0.13),
                ],
                normal,
                0.96,
            )
    for k in range(6):
        a = tau * k / 6
        s.plank(
            (3.8 * cos(a), 0, 3.8 * sin(a)),
            (23.1 * cos(a), 0, 23.1 * sin(a)),
            5.0,
            4.6,
            (0, 1, 0),
            OXIDE,
            1.0,
        )
    shaft(s, (0, -8.4, 0), (0, 8.4, 0), 6.8, OXIDE, 28)
    # An old ochre indexing mark makes even slow wheel rotation legible.
    s.plank((6.9, 3.0, 0), (21.8, 3.0, 0), 2.4, 0.30, (0, 0, 1), PAINT, 0.08)
    return s


def carriage():
    s = Joinery()
    for side in (-1, 1):
        y = side * 9.8
        # Slender cheeks leave the open spoke wheel silhouette legible from either bank.
        s.plank((0, y, 28), (0, y, -38), 7.1, 3.5, (1, 0, 0), OXIDE, 0.8)
        s.plank((-15.6, y, -29), (0, y, 24), (5.7), 3.2, (0, side, 0), OXIDE, 0.65)
        s.plank((0, y, 24), (15.6, y, -29), 5.7, 3.2, (0, side, 0), OXIDE, 0.65)
        shaft(s, (0, y - side * 2, 25.9), (0, y + side * 4, 25.9), 5.4, STEEL, 6)
        for x in (-13, 13):
            shaft(s, (x, y - side * 2, -25), (x, y + side * 3, -25), 3.2, STEEL, 8)
    shaft(s, (0, -12.3, -36), (0, 12.3, -36), 4.1, STEEL)
    # Two solid drawbars, swivel pin and a broad empty bow instead of a fictional dangling wire.
    s.plank((0, 0, -34), (0, 0, -67), 9, 5, (1, 0, 0), OXIDE, 1.1)
    for yy in (-3.6, 3.6):
        s.plank((0, yy, -58), (0, yy, -90), 6, 2.5, (1, 0, 0), STEEL, 0.6)
    shaft(s, (0, -7.5, -87), (0, 7.5, -87), 3.9, STEEL, 8)
    # The swivel carries an open, upturned horn. A cast loop can clear the
    # ochre tip and settle into the broad bottom of the hook.
    horn = [(0, 0, -88), (0, 3, -96), (0, 2, -102), (0, -3, -106),
            (0, -10, -107), (0, -21, -103), (0, -28, -94), (0, -29, -84)]
    tube(s, STEEL, [V(*p) for p in horn], 3.3, False, 12)
    shaft(s, (0, -28.8, -87), (0, -29.3, -82.6), 3.5, PAINT, 12)
    # A surviving flash of paint catches moonlight on the lower attachment.
    for z in (-48, -52):
        s.plank((-4.3, -2.8, z), (4.3, -2.8, z), 1.8, 0.35, (0, 0, 1), PAINT, 0.12)
    return s


def sling():
    s = Joinery()
    # Origin is the height of the iron bowl. The bight bears on the saddle,
    # with its two long legs returning through a bound lower handhold.
    path = [(-5,-7,0), (0,-7,3), (5,-7,0), (5,-5,-10), (2,5,-42),
            (0,6,-193), (0,6,-214), (0,4,-223), (0,0,-226),
            (0,-4,-223), (0,-6,-214), (0,-6,-193), (-3,-7,-42), (-5,-8,-10),
            (-5,-7,0)]
    tube(s, HEMP, [V(*p) for p in path[:-1]], 1.8, True, 12)
    for z in (-8, -11, -14):
        ellipse(s, HEMP, (0,-6,z), (7,0,0), (0,4,0), 1.5, 20)
    for z in (-184, -187, -190):
        ellipse(s, HEMP, (0,0,z), (3,0,0), (0,7.5,0), 1.5, 20)
    for y in (-6, 6):
        for z in range(-217, -195, 3):
            ellipse(s, HEMP, (0,y,z), (2,0,0), (0,2,.55), .7, 16)
    return s


def casting_bight():
    s = Joinery()
    # Flattened loop with a short whipping, presented open on the way up.
    ellipse(s, HEMP, (0,0,0), (12,0,3.3), (0,15,0), 1.8, 36)
    for y in (-15, -13):
        ellipse(s, HEMP, (0,y,0), (2.1,0,0), (0,0,1.9), .75, 16)
    return s


def station(near):
    s = Joinery()
    R.seed(631 if near else 981)
    # Origin is directly below the corresponding bumper. Heights here are map heights.
    sign = -1 if near else 1
    x0 = -145 if near else 246
    z0 = (631 if near else 581) + x0 * ((-82.0 if near else -18.0) / SPAN)
    width = 155 if near else 169
    for side in (-1, 1):
        heel = (x0 + sign * 38, side * (width + 20), 22)
        head = (x0, side * (width - 42), z0 - 5)
        s.timber(heel, head, 18, facets=16)
        s.timber(
            (x0 + sign * 213, side * (width + 41), 26),
            (x0, side * (width - 35), z0 - 54),
            12,
            r2=13,
        )
        s.timber(
            (x0 - sign * 85, side * (width + 11), 20),
            (x0, side * (width - 27), z0 - 99),
            10,
        )
        # Thick hewn knees connect the transom to the post.
        s.plank(
            (x0, side * (width - 38), z0 - 98),
            (x0, side * (width - 87), z0 - 14),
            13.4,
            15,
            (1, 0, 0),
            TIMBER,
            1.3,
        )
        for dz in (27, 61):
            y = side * (width - 39)
            shaft(s, (x0 - 21, y, z0 - dz), (x0 + 21, y, z0 - dz), 3.6, STEEL, 6)
    s.timber((x0, -width - 13, z0 - 20), (x0, width + 13, z0 - 18.5), 17, facets=16)
    # A second, higher cable saddle brings the working line clear of the cap.
    s.plank((x0, -40, z0 - 5), (x0, 40, z0 - 5), 24, 13, (1, 0, 0), TIMBER, 1.8)
    for yy in (-5.5, 5.5):
        s.plank(
            (x0 - 12, yy, z0 + 1.2),
            (x0 + 12, yy, z0 + 1.2),
            4.0,
            2.4,
            (0, 1, 0),
            STEEL,
            0.5,
        )
    for yy in (-32, 32):
        s.plank((x0 - 14, yy, z0), (x0 + 14, yy, z0), 5, 2, (0, 1, 0), OXIDE)
    # Guys meet two visibly buried timber deadmen clear of the path at walking height.
    for side in (-1, 1):
        guy = V(x0 + sign * 190, side * (width + 169), 52)
        head = V(x0, side * (width - 40), z0 - 19)
        s.timber((guy.x - 45, guy.y - 20, 30), (guy.x + 54, guy.y + 23, 31), 23)
        tube(s, STEEL, [head, (head + guy) * 0.5 - V(0, 0, 4), guy], 1.55)
        for k in (-1, 0, 1):
            ellipse(
                s,
                STEEL,
                (guy.x + 8 * k, guy.y + 3 * k, 33),
                (0, 22, 0),
                (0, 0, 23),
                1.4,
                26,
            )
    # The main wire sweeps behind the saddle into a bound deadman below the forest floor.
    anchor = x0 + sign * 208
    s.timber((anchor - 23, -224, 27), (anchor + 24, -126, 30), 20, facets=16)
    for offset in (-5, 0, 5):
        ellipse(
            s,
            STEEL,
            (anchor + offset, -172 + 2 * offset, 29),
            (18, -8, 0),
            (0, 0, 19),
            1.65,
            28,
        )
    # Hand-hewn plank walk. The near ramp follows a continuous collision surface.
    if near:
        a, b = -268.0, 52.0

        def deck(x):
            return min(123.0, 49.0 + 0.37 * max(0.0, x + 268.0))

        w = 79.0
    else:
        a, b = (0.86 - 1) * SPAN - 7, 147.0

        def deck(x):
            return 60.0 - 12.0 * min(1.0, max(0.0, (x - 76.0) / 71.0))

        w = 93
    n = round((b - a) / 18.6)
    for i in range(n):
        x = a + (i + 0.5) * (b - a) / n
        jitter = R.uniform(-1.5, 1.5)
        z = deck(x) - 3.35
        s.plank(
            (x + jitter, -w + R.uniform(-1.8, 1.1), z),
            (x, w + R.uniform(-1.2, 2.0), z + 0.35),
            (b - a) / n - 0.65,
            6.7,
            (1, 0, 0),
            BOARD,
            0.58,
        )
        if i % 2 == 0:
            for yy in (-w + 17, w - 17):
                shaft(s, (x, yy, z + 3.2), (x, yy, z + 4.0), 1.3, IRON, 6)
    for yy in (-w + 14, w - 14):
        steps = [a, -68, b] if near else [a, 76, b]
        for l, r in zip(steps, steps[1:]):
            s.plank(
                (l, yy, deck(l) - 12),
                (r, yy, deck(r) - 12),
                13,
                17,
                (0, 1, 0),
                TIMBER,
                1.4,
            )
        for x in (-68, 31) if near else (a + 12, 23, 133):
            s.timber(
                (x, yy, -186 if not near and x < 0 else 13),
                (x, yy, deck(x) - 10),
                9,
                facets=12,
            )
    # Isolated outer handrail posts keep the bank mouth legible without enclosing the rider.
    side = -1 if near else 1
    xp = (-230, -116, -15) if near else (a + 8, -45, 110)
    for x in xp:
        y = side * (w + 4)
        s.timber((x, y, deck(x) - 19), (x, y, deck(x) + 70), 5.8, facets=12)
        for dz in (53, 60):
            shaft(s, (x, y, deck(x) + dz), (x, y, deck(x) + dz + 3), 6.0, PAINT, 12)
    for l, r in zip(xp, xp[1:]):
        z1, z2 = deck(l) + 56, deck(r) + 56
        tube(
            s,
            HEMP,
            [
                V(l, y, z1),
                V((l + r) / 2, y, min(z1, z2) + abs(z1 - z2) / 2 - 11),
                V(r, y, z2),
            ],
            1.85,
        )
    return s


def main():
    assets.make_directory(DEST)
    materials = [
        assets.load_asset(WOOD + "/" + name)
        for name in (
            "M_HewnLogs",
            "M_AdzedBoards",
            "M_CutEnds",
            "M_ClayChinking",
            "M_ForgedStraps",
        )
    ]
    materials.extend(
        [
            metal(
                "M_SkylineSteel", (0.105, 0.13, 0.142), (0.37, 0.39, 0.36), 0.52, 0.46
            ),
            metal("M_OxideRed", (0.17, 0.039, 0.016), (0.44, 0.14, 0.046), 0.34, 0.68),
            metal("M_WornOchre", (0.33, 0.19, 0.045), (0.75, 0.56, 0.21), 0.07, 0.67),
            assets.load_asset(WOOD + "/M_TwistedHemp"),
        ]
    )
    if not all(materials):
        raise RuntimeError("The skyline materials could not be loaded")
    for name, s in (
        ("SM_HeadStation", station(True)),
        ("SM_TailStation", station(False)),
        ("SM_TrolleySheave", sheave()),
        ("SM_TrolleyIronwork", carriage()),
        ("SM_HempStrop", sling()),
        ("SM_CastingBight", casting_bight()),
    ):
        bake(name, s, materials, DEST)
    u.log("SKYLINE ASSETS SAVED")


if __name__ == "__main__":
    main()
