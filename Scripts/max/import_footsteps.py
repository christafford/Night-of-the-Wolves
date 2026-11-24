"""Prepare Kenney grass and timber footfalls and import the ten mono one-shots."""
from pathlib import Path
import array
import subprocess
import wave

import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = root / "Art/Source/Max/Footsteps"
cache = root / "Intermediate/MaxFootsteps"
cache.mkdir(parents=True, exist_ok=True)
tasks = []
for material, prefix in (("grass", "Trail"), ("wood", "Timber")):
    for i in range(5):
        src = source / f"footstep_{material}_{i:03d}.ogg"
        wav = cache / f"{prefix}{i+1}.wav"
        # Trim microphone preroll down to the onset; keep the leaf scuff / board decay.
        audio = array.array("h")
        audio.frombytes(subprocess.check_output(["ffmpeg", "-v", "error", "-i", str(src), "-ac", "1",
                                                "-ar", "44100", "-af", "highpass=f=38", "-f", "s16le", "-"]))
        peak = max(map(abs, audio))
        start = max(0, next(j for j, amp in enumerate(audio) if abs(amp) > .018 * peak) - 120)
        end = min(len(audio), len(audio) - next(j for j, amp in enumerate(reversed(audio)) if abs(amp) > .003 * peak) + 1700)
        with wave.open(str(wav), "wb") as writer:
            writer.setparams((1, 2, 44100, 0, "NONE", "uncompressed"))
            writer.writeframes(array.array("h", (round(26100 * amp / peak) for amp in audio[start:end])).tobytes())
        task = unreal.AssetImportTask()
        task.set_editor_properties(dict(filename=str(wav), destination_path="/Game/NightOfTheWolves/Max/Footsteps",
                                        automated=True, replace_existing=True, save=True))
        tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
if any(not task.imported_object_paths for task in tasks):
    raise RuntimeError("Missing Max footstep")
unreal.log("MAX_FOOTSTEPS_IMPORTED")
