"""Import the mastered music loop with Unreal Editor's WAV factory."""
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
task = unreal.AssetImportTask()
task.set_editor_properties({
    "filename": str(project / "Art/Source/Music/SW_MoonlitPursuit.wav"),
    "destination_path": "/Game/NightOfTheWolves/Music",
    "automated": True,
    "replace_existing": True,
    "save": True,
})
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
score = unreal.load_asset("/Game/NightOfTheWolves/Music/SW_MoonlitPursuit")
if not isinstance(score, unreal.SoundWave) or not task.get_objects():
    raise RuntimeError("Import did not produce the Moonlit Pursuit SoundWave")
score.set_editor_properties({"looping": True, "compression_quality": 88})
if not unreal.EditorAssetLibrary.save_loaded_asset(score, False):
    raise RuntimeError("Could not save the loop settings")
unreal.log("PURSUIT SCORE READY " + score.get_path_name())
