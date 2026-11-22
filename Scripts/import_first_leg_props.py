"""Import first-leg original props, assign their painted vertex color, set ambient loops."""
from pathlib import Path
import unreal

dest='/Game/NightOfTheWolves/FirstLeg'
root=Path(unreal.Paths.project_dir())/'Art/Source/FirstLeg'
assets=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
mat=assets.load_asset(dest+'/M_Painted') if assets.does_asset_exist(dest+'/M_Painted') else tools.create_asset('M_Painted',dest,unreal.Material,unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_nanite",True)
mat.set_editor_property("used_with_instanced_static_meshes",True)
mel=unreal.MaterialEditingLibrary
if not mel.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR):
    vertex=mel.create_material_expression(mat,unreal.MaterialExpressionVertexColor,-200,0)
    mel.connect_material_property(vertex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
assets.save_loaded_asset(mat)
for path in sorted(root.glob('*')):
    if path.suffix not in ('.wav','.glb'): continue
    task=unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=str(path),destination_path=dest,automated=True,replace_existing=True,save=True))
    tools.import_asset_tasks([task])
    imported=[a for a in task.get_objects() if isinstance(a,(unreal.StaticMesh,unreal.SoundWave))]
    if len(imported)!=1: raise RuntimeError(str(path)+' '+str(task.get_objects()))
    obj=imported[0]
    wanted=dest+'/'+path.stem
    if obj.get_path_name()!=wanted+"."+path.stem:
        if not assets.rename_asset(obj.get_path_name(),wanted): raise RuntimeError(wanted)
    if isinstance(obj,unreal.StaticMesh):
        obj.set_material(0,mat)
        unreal.log('WOLFPROP '+obj.get_path_name()+' '+str(obj.get_bounds()))
    elif path.stem in ('Hearth','Creek'):
        obj.set_editor_property('looping',True)
    assets.save_loaded_asset(obj)
