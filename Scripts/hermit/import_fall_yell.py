"""Import the prepared Wilhelm take as the hermit's one-shot falling sound."""
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
task = unreal.AssetImportTask()
task.filename = str(project / "Art/Source/FirstLeg/Hermit/SC_HermitFallYell.wav")
task.destination_path = "/Game/NightOfTheWolves/FirstLeg/Hermit"
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
wave = unreal.EditorAssetLibrary.load_asset(task.destination_path + "/SC_HermitFallYell")
if not task.imported_object_paths or not isinstance(wave, unreal.SoundWave):
    raise RuntimeError("Hermit fall SoundWave import failed")
if wave.get_editor_property("looping") or abs(wave.duration - 1.36) > 0.001:
    raise RuntimeError("Expected the single 1.36 second Wilhelm take")
unreal.log(f"HERMIT FALL YELL saved {wave.get_path_name()} {wave.duration:.3f}s")
