"""Bake the UE tangent normal from the wolf undercoat swatch with Pillow and NumPy."""

from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


def main():
    folder = Path(__file__).resolve().parents[2] / "Art/Source/FirstLeg/Wolves"
    with Image.open(folder / "TimberUndercoat.png") as source:
        gray = np.asarray(source.convert("L"), dtype=np.uint8)

    # Wrap the filters across tile edges. Strand relief follows local contrast so
    # dark patches in the coat retain the same overall depth as the silver ones.
    margin = 10
    tile = Image.fromarray(np.pad(gray, margin, mode="wrap"))

    def blurred(radius):
        pixels = np.asarray(tile.filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32)
        return pixels[margin:-margin, margin:-margin] / 255.0

    height = blurred(0.65) - 0.65 * blurred(2.0) - 0.35 * blurred(3.0)
    du = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
    dv = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5
    slope = np.hypot(du, dv)
    scale = np.minimum(
        0.29 / max(float(np.percentile(slope, 95)), 1e-5),
        0.62 / np.maximum(slope, 1e-5),
    )
    normal = np.stack((-du * scale, -dv * scale, np.ones_like(du)), axis=-1)
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    Image.fromarray(np.rint((normal * 0.5 + 0.5) * 255).astype(np.uint8)).save(
        folder / "TimberUndercoatNormal.png"
    )


if __name__ == "__main__":
    main()
