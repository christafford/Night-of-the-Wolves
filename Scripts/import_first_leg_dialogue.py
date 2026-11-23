"""Import the six mastered dialogue cues and check the saved SoundWaves."""
from pathlib import Path
import wave

import unreal

names = ("BanditDemand", "BanditGold", "BanditStay", "HermitWarning", "HermitShout", "HermitPit")
project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
folder = "/Game/NightOfTheWolves/FirstLeg/Voices"
durations = {}
tasks = []
for name in names:
    source = project / "Art/Source/FirstLeg/Voices" / (name + ".wav")
    with wave.open(str(source), "rb") as audio:
        if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate()) != (1, 2, 24000):
            raise ValueError(f"{source.name} must be mono 24 kHz PCM16")
        durations[name] = audio.getnframes() / audio.getframerate()
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = folder
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name, task in zip(names, tasks):
    if not task.imported_object_paths:
        raise RuntimeError("Import failed for " + name)
    asset = unreal.EditorAssetLibrary.load_asset(folder + "/" + name)
    if not isinstance(asset, unreal.SoundWave) or asset.get_editor_property("looping"):
        raise RuntimeError("Expected non-looping SoundWave for " + name)
    duration = asset.get_editor_property("duration")
    if abs(duration - durations[name]) > 0.015:
        raise RuntimeError(f"{name} saved with duration {duration}, expected {durations[name]}")
    unreal.log(f"VOICE IMPORT {name} {duration:.3f}s")
unreal.log("FIRST LEG VOICES saved six sound waves")
