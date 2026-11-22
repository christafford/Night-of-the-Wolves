"""Composite the original lettering and moonlit forest at title-card resolution.

Requires ImageMagick, librsvg, and Noto Serif Display Regular and Italic.
"""
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

ASSETS = Path(__file__).resolve().parents[2] / "Art/Source/Presentation/Opening"


def run(*cmd):
    subprocess.run([str(part) for part in cmd], check=True)


def compose():
    with TemporaryDirectory() as temp:
        work = Path(temp)
        letters = work / "lettering.png"
        run("rsvg-convert", "--output", letters, ASSETS / "OpeningLettering.svg")
        # A faint contact-print grain in the silver lettering echoes the forest
        # plate. Preserve the clear outer contour and the fine strokes of the credit.
        grain = work / "grain.png"
        run("magick", "-size", "2560x1440", "xc:gray", "-seed", "1971",
            "+noise", "Random", "-colorspace", "gray", "-level", "3%,100%,1.6",
            "+level", "79%,100%", "-depth", "8", grain)
        mask, ink = work / "mask.png", work / "ink.png"
        run("magick", letters, "-alpha", "extract", mask)
        run("magick", letters, "-alpha", "off", grain, "-compose", "multiply",
            "-composite", mask, "-compose", "CopyOpacity", "-composite", ink)
        run("magick", ASSETS / "MoonlitPlate.png", "-resize", "2560x1440^",
            "-gravity", "center", "-extent", "2560x1440", ink,
            "-compose", "Over", "-composite", "-alpha", "off", "-colorspace", "sRGB",
            "-depth", "8", "-strip", "PNG24:" + str(ASSETS / "T_OpeningTitle.png"))


if __name__ == "__main__":
    compose()
