"""Centimetre-scale carpentry meshes. UE triangles wind clockwise from the exposed side."""

import math
import random
from collections import defaultdict

import unreal as u

V = u.Vector
TAU = math.tau
R = random.Random(97541)
TIMBER, BOARD, END, DAUB, IRON, SHAKE = range(6)


class Piece:
    def __init__(self):
        self.pos, self.uv, self.normals, self.colors, self.tris = [], [], [], [], []


class Joinery:
    def __init__(self):
        self.slots = defaultdict(Piece)

    def face(self, slot, points, coords, outward, tint, normals=None):
        part = self.slots[slot]
        base = len(part.pos)
        part.pos.extend(points)
        part.uv.extend(u.Vector2D(x, y) for x, y in coords)
        part.normals.extend(normals or [outward] * len(points))
        part.colors.extend([u.LinearColor(tint, tint, tint, 1)] * len(points))
        reverse = (points[1] - points[0]).cross(points[2] - points[0]).dot(outward) > 0
        for i in range(1, len(points) - 1):
            part.tris.append(
                u.IntVector(
                    base,
                    base + (i + 1 if reverse else i),
                    base + (i if reverse else i + 1),
                )
            )

    def timber(self, start, finish, radius, slot=TIMBER, ends=END, facets=18, r2=None):
        start, finish = V(*start), V(*finish)
        axis = (finish - start).normal()
        length = (finish - start).length()
        across = axis.cross(V(0, 0, 1) if abs(axis.z) < 0.93 else V(0, 1, 0)).normal()
        up = across.cross(axis).normal()
        depth = r2 or radius
        rings = max(2, int(length / 63))
        u0, v0 = R.random(), R.random()
        tint = R.uniform(0.78, 1)
        irregular = [R.uniform(0.94, 1.04) for j in range(facets)]
        phase = R.random() * TAU
        profile, ns = [], []
        for i in range(rings + 1):
            t = i / rings
            bend = min(radius * 0.09, length * 0.004) * math.sin(math.pi * t)
            center = (
                start
                + axis * (length * t)
                + across * (bend * math.sin(phase))
                + up * (bend * math.cos(phase))
            )
            ring, surf = [], []
            for j in range(facets):
                a = TAU * j / facets
                taper = (
                    (1 - 0.065 * t)
                    * irregular[j]
                    * (1 + 0.013 * math.sin(3 * a + t * 4 + phase))
                )
                ring.append(
                    center
                    + (across * (radius * math.cos(a)) + up * (depth * math.sin(a)))
                    * taper
                )
                surf.append(
                    (
                        across * (math.cos(a) / radius) + up * (math.sin(a) / depth)
                    ).normal()
                )
            profile.append(ring)
            ns.append(surf)
        for i in range(rings):
            for j in range(facets):
                k = (j + 1) % facets
                points = [
                    profile[i][j],
                    profile[i + 1][j],
                    profile[i + 1][k],
                    profile[i][k],
                ]
                normals = [ns[i][j], ns[i + 1][j], ns[i + 1][k], ns[i][k]]
                coords = [
                    (u0 + q / facets * 0.86, v0 + p / rings * length / 154)
                    for p, q in ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))
                ]
                self.face(
                    slot,
                    points,
                    coords,
                    (normals[0] + normals[3]).normal(),
                    tint,
                    normals,
                )
        for i, direction in ((0, -1), (rings, 1)):
            coords = [
                (
                    0.5 + 0.48 * math.cos(TAU * j / facets),
                    0.5 + 0.48 * math.sin(TAU * j / facets),
                )
                for j in range(facets)
            ]
            self.face(ends, profile[i], coords, axis * direction, tint)

    def plank(self, start, finish, breadth, thickness, across, slot=BOARD, bevel=0.6):
        """An eased rectilinear board, UV-mapped along the actual lengths of its eight faces."""
        a, b = V(*start), V(*finish)
        axis = (b - a).normal()
        side = V(*across).normal()
        face = axis.cross(side).normal()
        w, h = breadth / 2, thickness / 2
        e = min(bevel, w * 0.35, h * 0.42)
        cross_section = [
            (-w + e, -h),
            (w - e, -h),
            (w, -h + e),
            (w, h - e),
            (w - e, h),
            (-w + e, h),
            (-w, h - e),
            (-w, -h + e),
        ]
        ring = [side * x + face * y for x, y in cross_section]
        u0 = R.random()
        # Each physical board takes its grain from a single wide plank of the scan.
        v0 = (
            R.choice((0.072, 0.198, 0.338, 0.46, 0.59, 0.717, 0.84, 0.959))
            + R.uniform(-0.013, 0.013)
        ) % 1
        tint = R.uniform(0.76, 1)
        for j, off in enumerate(ring):
            k = (j + 1) % len(ring)
            other = ring[k]
            normal = (other - off).cross(axis).normal()
            if normal.dot(off + other) < 0:
                normal = -normal
            du = (b - a).length() / 187
            dv = min(0.092, (other - off).length() / 240)
            coords = [(u0, v0), (u0 + du, v0), (u0 + du, v0 + dv), (u0, v0 + dv)]
            if slot == TIMBER:
                coords = [(v, u) for u, v in coords]
            self.face(
                slot, [a + off, b + off, b + other, a + other], coords, normal, tint
            )
        for center, normal in ((a, -axis), (b, axis)):
            cap = [
                (
                    0.5 + x / max(breadth, thickness) * 0.91,
                    0.5 + y / max(breadth, thickness) * 0.91,
                )
                for x, y in cross_section
            ]
            self.face(
                IRON if slot == IRON else END,
                [center + r for r in ring],
                cap,
                normal,
                tint,
            )

    def mesh(self):
        m = u.DynamicMesh()
        for slot, data in self.slots.items():
            m.append_buffers_to_mesh(
                u.GeometryScriptSimpleMeshBuffers(
                    vertices=data.pos,
                    triangles=data.tris,
                    uv0=data.uv,
                    normals=data.normals,
                    vertex_colors=data.colors,
                ),
                slot,
            )
        return m


def shell():
    R.seed(97541)
    s = Joinery()
    front, back, wall = -421, 398, 396
    # The side courses are half a diameter out of phase with the gable courses.
    # Their ends project past the crossing, exposing staggered cut faces at the corners.
    for row in range(13):
        z = 11 + 27 * row
        rr = R.uniform(17.1, 18.5)
        for x in (front, back):
            extent = wall + R.uniform(29, 52)
            spans = [(-extent, extent)]
            if x == front and z < 326:
                spans = [(-extent, -94), (93, extent)]
                if 128 < z < 253:
                    spans = [(-extent, -94), (93, 166), (274, extent)]
            for lo, hi in spans:
                s.timber((x, lo, z), (x, hi, z + R.uniform(-1.4, 1.4)), rr, r2=17.5)
                # An inset, narrow roll of clay chinking seals the seam between trunks.
                if row < 12:
                    s.plank(
                        (x, lo, z + 13.7),
                        (x, hi, z + 13.7),
                        25,
                        8,
                        (1, 0, 0),
                        DAUB,
                        1.6,
                    )
        for side in (-1, 1):
            s.timber(
                (front - R.uniform(25, 39), wall * side, z + 13),
                (back + R.uniform(22, 38), wall * side, z + 13),
                rr,
                r2=17.5,
            )
            if row < 12:
                s.plank(
                    (front, wall * side, z + 26.7),
                    (back, wall * side, z + 26.7),
                    25,
                    8,
                    (0, 1, 0),
                    DAUB,
                    1.6,
                )
    for x in (front, back):
        for row in range(7):
            z = 354 + row * 27
            width = (543 - z) / 0.455
            s.timber((x, -width, z), (x, width, z), 16, r2=17)
    # Hand-hewn boxed ends contain the short logs at the door and the window.
    for y in (-98, 98):
        s.plank((front + 5, y, 0), (front + 5, y, 337), 54, 16, (1, 0, 0), TIMBER, 2.2)
    s.plank((front + 5, -111, 334), (front + 5, 111, 334), 55, 21, (1, 0, 0), TIMBER, 2)
    for y in (164, 276):
        s.plank(
            (front + 3, y, 133), (front + 3, y, 248), 46, 12, (1, 0, 0), TIMBER, 1.5
        )
    for z in (136, 245):
        s.plank((front + 3, 155, z), (front + 3, 285, z), 54, 11, (1, 0, 0), BOARD, 1.2)
    # A mortised cleat bridges the rear wall's uneven log faces behind the rope peg.
    for y in (-126, -44):
        s.plank((360, y, 142), (392, y, 142), 22, 22, (0, 0, 1), TIMBER, 1.5)
    s.plank((356, -144, 144), (356, -22, 144), 18, 9, (0, 0, 1), bevel=1.4)
    # A floor inside the sill makes the transition from bare earth visible on the jump.
    for i in range(32):
        y = -392 + i * 24.8
        cut = R.uniform(-125, 196)
        for lo, hi in ((-398, cut), (cut + 0.65, 399)):
            s.plank((lo, y, 0.1), (hi, y, 0.1), 24.45, 1.8, (0, 1, 0), bevel=0.45)
    # Exposed collar ties, rafters and overlapping hand-split shingles under the ridge.
    for x in (-422, -286, -145, 0, 145, 282, 399):
        s.timber((x, -410, 343), (x, 410, 343), 11)
        for side in (-1, 1):
            s.timber((x, side * 450, 335), (x, 0, 535), 10)
    s.timber((-461, 0, 538), (438, 0, 538), 14)
    for side in (-1, 1):
        s.plank(
            (-467, side * 448, 331),
            (446, side * 448, 331),
            25,
            9,
            (0, 0, 1),
            TIMBER,
            1.3,
        )
        for row in range(10):
            y0 = 1 + row * 46
            y1 = min(461, y0 + 61)
            x = -469
            while x < 445:
                width = min(445 - x, R.uniform(18, 31))
                if width < 0.9:
                    break
                xc = x + width * 0.5
                # The lower edge overhangs the next course. Offsetting composition by
                # course eliminates straight seams and gives the roof a ragged drip line.
                lo_z = 548 - y0 * 0.455 + 2.1
                hi_z = 548 - y1 * 0.455 + 4.8 + R.uniform(-0.8, 0.8)
                s.plank(
                    (xc, side * y0, lo_z),
                    (xc + R.uniform(-0.3, 0.3), side * y1, hi_z),
                    width - 0.21,
                    4.5,
                    (1, 0, 0),
                    SHAKE,
                    0.5,
                )
                x += width
    return s


def door():
    d = Joinery()
    # Local origin matches the middle of the original moving collision panel.
    cuts = [-89, -67, -49, -27, -6, 13, 34, 56, 73, 89]
    for a, b in zip(cuts, cuts[1:]):
        y = (a + b) / 2
        offset = R.uniform(-0.7, 0.7)
        d.plank(
            (offset, y, -161),
            (offset, y, 161.0 - R.uniform(0, 0.9)),
            b - a - 0.3,
            14,
            (0, 1, 0),
            bevel=0.77,
        )
    # Through-tenoned ledges and a diagonal brace on the hearth-facing side.
    for z in (-106, 91):
        d.plank((10, -85, z), (10, 85, z), 21, 7, (0, 0, 1), bevel=1.2)
    d.plank((10, -72, -94), (10, 73, 79), 13, 6, (0, 0.765, -0.642), bevel=0.8)
    for z in (-102, 101):
        # A blacksmith's long tapered straps, stepped backplates, knuckles and domed rivets.
        d.plank((-9, -86, z), (-9, -66, z), 12, 2.2, (0, 0, 1), IRON, 0.8)
        d.plank((-9, -70, z), (-9, -3, z), 6.5, 2.0, (0, 0, 1), IRON, 0.8)
        d.plank((-9, -5, z), (-9, 12, z), 3.6, 2, (0, 0, 1), IRON, 0.65)
        d.timber((-10, -87, z - 7), (-10, -87, z + 7), 3.7, IRON, IRON, 12)
        for y in (-77, -53, -26, 5):
            d.timber((-10.2, y, z), (-12.3, y, z), 1.9, IRON, IRON, 10)
    for side in (-1, 1):
        for z in (-52, -26):
            d.plank(
                (side * 8, 63, z - 4), (side * 8, 63, z + 4), 9, 2, (0, 1, 0), IRON, 0.6
            )
        # A bent iron pull stands clear of the door in silhouette on either side.
        path = [
            (side * 8, 63, -52),
            (side * 15, 63, -50),
            (side * 18, 63, -46),
            (side * 18, 63, -32),
            (side * 15, 63, -28),
            (side * 8, 63, -26),
        ]
        for p, q in zip(path, path[1:]):
            d.timber(p, q, 1.9, IRON, IRON, 9)
    return d
