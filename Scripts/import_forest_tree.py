"""Import the source tree mesh through Unreal's FBX importer."""
import os
import unreal

ui = unreal.FbxImportUI()
ui.set_editor_property("automated_import_should_detect_type", False)
ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
ui.set_editor_property("import_mesh", True)
ui.set_editor_property("import_materials", True)
ui.set_editor_property("import_textures", True)
data = ui.get_editor_property("static_mesh_import_data")
data.set_editor_property("combine_meshes", True)
data.set_editor_property("build_nanite", True)
task = unreal.AssetImportTask()
task.set_editor_property("filename", os.path.join(unreal.Paths.project_dir(), "Art/Source/ForestTrees/island_tree_02.fbx"))
task.set_editor_property("destination_path", "/Game/NightOfTheWolves/ForestTrees")
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("replace_existing", False)
task.set_editor_property("options", ui)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
paths = task.get_editor_property("imported_object_paths")
unreal.log("FOREST TREE IMPORT: " + str(paths))
if not paths:
    raise RuntimeError("FBX importer returned no assets")
