"""Import the death-card snarl as a one-shot SoundWave through the Unreal Editor."""

from pathlib import Path

import unreal as u

folder = "/Game/NightOfTheWolves/Presentation"
name = "WolfCardSnarl"
project = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
source = project / "Art/Source/Presentation" / (name + ".wav")
task = u.AssetImportTask()
task.set_editor_properties(
    {
        "filename": str(source),
        "destination_path": folder,
        "replace_existing": True,
        "automated": True,
        "save": True,
    }
)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
wave = u.load_asset(folder + "/" + name)
if not isinstance(wave, u.SoundWave) or not task.get_objects():
    raise RuntimeError("Editor did not import the death-card snarl")
wave.set_editor_properties({"looping": False, "compression_quality": 85})
if not u.EditorAssetLibrary.save_loaded_asset(wave, False):
    raise RuntimeError("Editor could not save the death-card snarl")
u.log("DEATH CARD SNARL READY " + wave.get_path_name())
