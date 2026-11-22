"""Import the pre-rendered dialogue as cooked, non-looping Sound Waves."""
from pathlib import Path
import unreal

tasks = []
for source in sorted((Path(unreal.Paths.project_dir())/'Art/Source/FirstLeg/Voices').glob('*.wav')):
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = '/Game/NightOfTheWolves/FirstLeg/Voices'
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
if len(tasks) != 6 or any(not task.imported_object_paths for task in tasks):
    raise RuntimeError('Failed to import all six dialogue lines')
unreal.log('FIRST LEG VOICES saved six sound waves')
