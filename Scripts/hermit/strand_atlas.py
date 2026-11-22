"""Eight whisker masks, from broken grizzled wisps to isolated flyaways."""

from math import pi, sin
from pathlib import Path
from random import Random

from PIL import Image, ImageDraw


def build(path):
    rng = Random(1870)
    k = 3
    atlas = Image.new("RGB", (1024 * k, 512 * k))
    draw = ImageDraw.Draw(atlas)
    for col, num in enumerate((22, 25, 19, 27, 12, 15, 3, 4)):
        for i in range(num):
            u = (i + rng.uniform(0.15, 0.85)) / num
            phase = rng.uniform(0, 2 * pi)
            start = rng.uniform(0.008, 0.17) if num > 5 else rng.uniform(0.006, 0.05)
            end = rng.uniform(0.61, 0.996) if num > 5 else rng.uniform(0.90, 0.999)
            color = rng.randrange(38, 243)
            width = rng.uniform(1.5, 3.3) if num > 5 else rng.uniform(8, 16)

            def point(t):
                spread = (u - 0.5) * 101 * (0.68 + 0.32 * sin(pi * t))
                bend = sin(t * 8 + phase) * (2.6 + 2 * t) + 3.7 * sin(t * 3 + col) * t
                return (
                    (col * 128 + 64 + spread + bend) * k,
                    (14 + (start + (end - start) * t) * 480) * k,
                )

            p = point(0)
            for j in range(1, 34):
                t = j / 33
                q = point(t)
                taper = min(1, 0.21 + t * 9) * min(1, 0.14 + (1 - t) * 5)
                draw.line(
                    [p, q], fill=(255, color, int(20 + 224 * t)), width=max(1, round(width * k * taper))
                )
                p = q
    path.parent.mkdir(parents=True, exist_ok=True)
    atlas.resize((1024, 512), Image.Resampling.LANCZOS).save(path)


if __name__ == "__main__":
    build(Path(__file__).resolve().parents[2] / "Art/Source/FirstLeg/Hermit/T_WhiskerStrands.png")
