"""Download the two CC0 Poly Haven scans used for the log shell and door."""

import hashlib
import json
from pathlib import Path
import subprocess

OUT = Path(__file__).resolve().parents[2] / "Art/Source/FirstLeg/Carpentry"


def fetch(url):
    return subprocess.check_output(["curl", "-fLsS", "--retry", "2", url])


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True, parents=True)
    for asset, prefix in (("rough_wood", "Hewn"), ("brown_planks_09", "Sawn")):
        files = json.loads(fetch(f"https://api.polyhaven.com/files/{asset}"))
        for channel, label in (
            ("Diffuse", "Color"),
            ("nor_dx", "Normal"),
            ("Rough", "Roughness"),
        ):
            record = files[channel]["2k" if label != "Roughness" else "1k"]["jpg"]
            data = fetch(record["url"])
            if hashlib.md5(data).hexdigest() != record["md5"]:
                raise RuntimeError(f"Download checksum mismatch for {asset} {channel}")
            (OUT / f"T_{prefix}{label}.jpg").write_bytes(data)
