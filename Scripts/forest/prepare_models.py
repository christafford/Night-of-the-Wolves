"""Prepare compact conifer bough atlases, trunks, and a scanned fern from Poly Haven sources."""

import json
import math
import random
import struct
import subprocess
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[2] / "Art/Source/Cascades"
ROOT.mkdir(exist_ok=True)
TAU = math.tau


def source(asset, field):
    data = subprocess.check_output(["curl", "-fsSL", "https://api.polyhaven.com/files/" + asset])
    info = json.loads(data)
    image = ROOT / (asset + "_" + field + ".jpg")
    if not image.exists():
        subprocess.run(["curl", "-fsSL", info[field]["1k"]["jpg"]["url"], "-o", image], check=True)
    return image


def bough(name, box):
    diffuse = Image.open(source(name, "twig_diff")).convert("RGBA")
    alpha = Image.open(source(name, "twig_alpha")).convert("L")
    diffuse.putalpha(alpha)
    twig = diffuse.crop([int(x * diffuse.width) for x in box])
    rng = random.Random(4701)
    out = Image.new("RGBA", (768, 1024))
    stem = ImageDraw.Draw(out)
    for i in range(16):
        t = i / 17
        y = int(985 - t * 910)
        x = int(383 + 16 * math.sin(t * 3.5))
        stem.line((x, y, x + 1, y - 60), fill=(73, 60, 42, 255), width=int(12 * (1 - t)) + 3)
        for sign in [-1, 1]:
            length = int((215 + 70 * math.sin(t * 3.5)) * (1 - t * 0.74) * rng.uniform(0.85, 1.12))
            angle = sign * rng.uniform(36, 65) * (1 - t * 0.45)
            sprite = twig.resize((int(length * 0.76), length), Image.Resampling.LANCZOS)
            pivot = (int(sprite.width * 0.49), int(sprite.height * 0.95))
            tile = Image.new("RGBA", (2 * length + 8, 2 * length + 8))
            tile.alpha_composite(sprite, (length - pivot[0], length - pivot[1]))
            tile = tile.rotate(angle, resample=Image.Resampling.BICUBIC, center=(length, length))
            out.alpha_composite(tile, (x - length, y - length))
    # Trim transparent margins to use texture bandwidth for foliage.
    out = out.crop(out.getbbox()).resize((512, 1024), Image.Resampling.LANCZOS)
    out.save(ROOT / (name + "_bough.png"))


def export(name, groups):
    blob = bytearray()
    views = []
    access = []
    primitives = []
    mats = []

    def stream(values, fmt, shape):
        n = len(values[0])
        ofs = len(blob)
        pack = struct.Struct("<" + fmt * n)
        for v in values:
            blob.extend(pack.pack(*v))
        size = len(blob) - ofs
        while len(blob) % 4:
            blob.append(0)
        views.append({"buffer": 0, "byteOffset": ofs, "byteLength": size})
        access.append(
            {
                "bufferView": len(views) - 1,
                "componentType": 5126 if fmt == "f" else 5125,
                "count": len(values),
                "type": shape,
                "min": [min(v[i] for v in values) for i in range(n)],
                "max": [max(v[i] for v in values) for i in range(n)],
            }
        )
        return len(access) - 1

    for material, (vertices, uvs, triangles) in groups.items():
        if not triangles:
            continue
        # Source geometry is Z-up; glTF is right-handed and Y-up.
        pos = stream([(x, z, -y) for x, y, z in vertices], "f", "VEC3")
        uv = stream(uvs, "f", "VEC2")
        indices = stream([(i,) for tri in triangles for i in tri], "I", "SCALAR")
        primitives.append(
            {
                "attributes": {"POSITION": pos, "TEXCOORD_0": uv},
                "indices": indices,
                "material": len(mats),
            }
        )
        mats.append(
            {"name": material, "doubleSided": True, "pbrMetallicRoughness": {"metallicFactor": 0}}
        )
    header = {
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"name": name, "mesh": 0}],
        "meshes": [{"name": name, "primitives": primitives}],
        "materials": mats,
        "accessors": access,
        "bufferViews": views,
        "buffers": [{"byteLength": len(blob)}],
    }
    data = json.dumps(header, separators=(",", ":")).encode()
    data += b" " * (-len(data) % 4)
    (ROOT / (name + ".glb")).write_bytes(
        struct.pack("<4sII", b"glTF", 2, 28 + len(data) + len(blob))
        + struct.pack("<I4s", len(data), b"JSON")
        + data
        + struct.pack("<I4s", len(blob), b"BIN\0")
        + blob
    )


def canopy(name, seed, pine=False):
    r = random.Random(seed)
    h = 18.4 if pine else 16.9
    bark = ([], [], [])
    needles = ([], [], [])

    def vertex(group, point, uv):
        group[0].append(point)
        group[1].append(uv)
        return len(group[0]) - 1

    def strip(group, rows, width):
        for a, b in zip(rows, rows[1:]):
            for j in range(width - 1):
                group[2].extend([(a[j], a[j + 1], b[j + 1]), (a[j], b[j + 1], b[j])])

    def trunk(z):
        t = z / h
        return ((0.48 if pine else 0.22) * t * t + 0.11 * math.sin(5 * t), 0.16 * math.sin(3 * t), z)

    rows = []
    for t in [0, 0.014, 0.038, 0.075, 0.14, 0.24, 0.36, 0.50, 0.66, 0.81, 0.93, 1.0]:
        x, y, z = trunk(t * h)
        ring = []
        for j in range(17):
            a = TAU * j / 16
            rr = (0.35 * (1 - t) ** 0.75 + 0.012) * (1 + 0.075 * math.sin(a * 3 + t * 1.7)) + 0.19 * max(
                0, 1 - 12 * t
            ) ** 2 * (0.5 + 0.5 * math.sin(5 * a)) ** 2
            ring.append(
                vertex(
                    bark, (x + rr * math.cos(a), y + rr * math.sin(a), z - 0.10), (j / 8, 1 - z / 2.3)
                )
            )
        rows.append(ring)
    strip(bark, rows, 17)
    # Each bough has a woody taper, a curved spine, irregular droop and a folded surface.
    layers = 27 if not pine else 21
    for i in range(layers):
        t = (i + 0.3) / layers
        z = h * (0.065 + 0.92 * t)
        mature = t > (0.17 if not pine else 0.34)
        offset = r.uniform(0, TAU)
        count = r.randint(5, 10) if mature else r.randint(2, 5)
        for j in range(count):
            angle = offset + TAU * (j + r.uniform(-0.30, 0.30)) / count
            zz = z + r.uniform(-0.16, 0.16)
            length = (0.42 + (3.85 if not pine else 4.5) * (1 - t) ** 0.66) * r.uniform(0.67, 1.17)
            if not mature:
                length *= r.uniform(0.24, 0.48)
            dx, dy = math.cos(angle), math.sin(angle)
            bend = r.uniform(-0.29, 0.29)
            drop = r.uniform(-0.28, -0.06) if not pine else r.uniform(-0.10, 0.21)
            bank = r.uniform(-0.27, 0.27)

            def at(f):
                x, y, _ = trunk(zz)
                return (
                    x + dx * f * length - dy * bend * math.sin(f * 2.3),
                    y + dy * f * length + dx * bend * math.sin(f * 2.3),
                    zz + length * (drop * f - 0.055 * math.sin(f * math.pi)),
                )

            rows = []
            for f in (0, 0.38, 0.75, 1):
                x, y, z0 = at(f)
                rr = (0.06 * (1 - f) + 0.009) * (1 - t * 0.6)
                rows.append(
                    [
                        vertex(
                            bark,
                            (
                                x - dy * rr * math.cos(k * TAU / 5),
                                y + dx * rr * math.cos(k * TAU / 5),
                                z0 + rr * math.sin(k * TAU / 5),
                            ),
                            (k / 5, f * 2),
                        )
                        for k in range(6)
                    ]
                )
            strip(bark, rows, 6)
            if not mature:
                continue
            # Alternating sprays fan out from a woody leader, with visible sky between them.
            # A folded card models each spray, so a view up through the crown still sees needles.
            for f, sign in [
                (0.20, -1),
                (0.31, 1),
                (0.46, -1),
                (0.50, 1),
                (0.68, -1),
                (0.71, 1),
                (0.80, 0),
            ]:
                base = at(f)
                heading = angle + sign * r.uniform(0.47, 0.94)
                vx, vy = math.cos(heading), math.sin(heading)
                shoot = length * (0.35 + 0.43 * (1 - f)) * r.uniform(0.73, 1.18)
                spread = shoot * r.uniform(0.23, 0.36)
                sag = r.uniform(-0.18, 0.12)
                rows = []
                for part in (0, 0.48, 1):
                    sx = base[0] + vx * shoot * part
                    sy = base[1] + vy * shoot * part
                    sz = base[2] + shoot * (sag * part - 0.065 * math.sin(part * math.pi))
                    width = spread * (1 - 0.33 * part)
                    rows.append(
                        [
                            vertex(
                                needles,
                                (
                                    sx - vy * width * k,
                                    sy + vx * width * k,
                                    sz - abs(k) * width * 0.22 + bank * k * width,
                                ),
                                ((1 + k) * 0.5, 1 - part),
                            )
                            for k in (-1, 0, 1)
                        ]
                    )
                strip(needles, rows, 3)
    export(name, {"Cambium": bark, "Needles": needles})


def scanned_mesh(source_name, name, material, variant=0, centered_meter=False):
    info = json.loads(
        subprocess.check_output(["curl", "-fsSL", "https://api.polyhaven.com/files/" + source_name])
    )["gltf"]["1k"]["gltf"]
    scene = json.loads(subprocess.check_output(["curl", "-fsSL", info["url"]]))
    binary = next(v for k, v in info["include"].items() if k.endswith(".bin"))
    raw = subprocess.check_output(["curl", "-fsSL", binary["url"]])

    def unpack(n):
        acc = scene["accessors"][n]
        view = scene["bufferViews"][acc["bufferView"]]
        fmt = {5126: "f", 5123: "H", 5125: "I"}[acc["componentType"]] * {
            "VEC3": 3,
            "VEC2": 2,
            "SCALAR": 1,
        }[acc["type"]]
        st = struct.Struct("<" + fmt)
        stride = view.get("byteStride", st.size)
        ofs = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
        return [st.unpack_from(raw, ofs + i * stride) for i in range(acc["count"])]

    primitive = scene["meshes"][variant]["primitives"][0]
    uv = unpack(primitive["attributes"]["TEXCOORD_0"])
    positions = unpack(primitive["attributes"]["POSITION"])
    if centered_meter:
        lo = [min(p[i] for p in positions) for i in range(3)]
        hi = [max(p[i] for p in positions) for i in range(3)]
        diameter = max(b - a for a, b in zip(lo, hi))
        positions = [tuple((p[i] - (lo[i] + hi[i]) / 2) / diameter for i in range(3)) for p in positions]
    ix = unpack(primitive["indices"])
    export(
        name,
        {
            material: (
                [(x, -z, y) for x, y, z in positions],
                uv,
                [tuple(v[0] for v in ix[i : i + 3]) for i in range(0, len(ix), 3)],
            )
        },
    )


def fern():
    scanned_mesh("fern_02", "SM_SwordFern", "Fronds")
    rgba = Image.open(source("fern_02", "Diffuse")).convert("RGBA")
    rgba.putalpha(Image.open(source("fern_02", "Alpha")).convert("L"))
    rgba.save(ROOT / "FernFronds.png")


def creek_rocks():
    for i, name in enumerate(("SM_CreekStoneA", "SM_CreekStoneB", "SM_CreekStoneC")):
        scanned_mesh("rock_moss_set_01", name, "StreamStone", i, centered_meter=True)
    for name in ("rock_moss_set_01", "dry_riverbed_rock"):
        source(name, "Diffuse")
        source(name, "nor_dx")


if __name__ == "__main__":
    bough("fir_tree_01", (0.302, 0.392, 0.641, 0.777))
    bough("pine_tree_01", (0.032, 0.032, 0.213, 0.444))
    source("fir_tree_01", "bark_diff")
    source("pine_tree_01", "bark_diff")
    canopy("SM_WesternFir", 1503)
    canopy("SM_ShorePine", 221, True)
    fern()
    scanned_mesh("dead_tree_trunk_02", "SM_NurseLog", "WeatheredWood")
    source("dead_tree_trunk_02", "Diffuse")
    creek_rocks()
