"""Import the photographed yard tools and the bound bale into the shipping content folder."""
from pathlib import Path
import unreal as u

ROOT = '/Game/NightOfTheWolves/FirstLeg/CottageYard'
SOURCE = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())) / 'Art/Source/FirstLeg/CottageYard'
library = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
edit = u.MaterialEditingLibrary


def import_asset(file, destination):
    name = Path(file).stem
    desired = destination + '/' + name
    task = u.AssetImportTask()
    task.set_editor_properties(dict(filename=str(SOURCE / file), destination_path=destination,
                                   automated=True, save=True, replace_existing=True))
    tools.import_asset_tasks([task])
    asset = next((obj for obj in task.get_objects()
                  if isinstance(obj,(u.StaticMesh,u.Texture2D)) and obj.get_name()==name), None)
    if not asset:
        raise RuntimeError('Missing imported asset ' + desired)
    if asset.get_path_name().split('.')[0] != desired:
        if not library.rename_asset(asset.get_path_name(), desired):
            raise RuntimeError('Could not move ' + asset.get_path_name())
    return asset


def material(name):
    path = ROOT + '/M_' + name
    mat = library.load_asset(path) if library.does_asset_exist(path) else tools.create_asset(
        'M_'+name, ROOT, u.Material, u.MaterialFactoryNew())
    mat.set_editor_property("used_with_nanite", True)
    edit.delete_all_material_expressions(mat)
    return mat


def connect(node, socket, prop):
    if not edit.connect_material_property(node, socket, prop):
        raise RuntimeError('Could not connect ' + str(prop))


def surface(name):
    mat=material(name)
    for row, (suffix, compression, sampler) in enumerate([
        ('Color',u.TextureCompressionSettings.TC_DEFAULT,u.MaterialSamplerType.SAMPLERTYPE_COLOR),
        ('Normal',u.TextureCompressionSettings.TC_NORMALMAP,u.MaterialSamplerType.SAMPLERTYPE_NORMAL),
        ('ORM',u.TextureCompressionSettings.TC_MASKS,u.MaterialSamplerType.SAMPLERTYPE_MASKS)]):
        path=next(SOURCE.glob('T_'+name+'_'+suffix+'.*'))
        tex=import_asset(path.name, ROOT+'/Textures')
        tex.set_editor_properties(dict(compression_settings=compression,srgb=suffix=='Color'))
        library.save_loaded_asset(tex)
        sample=edit.create_material_expression(mat,u.MaterialExpressionTextureSample,-480,(row-1)*190)
        sample.set_editor_properties(dict(texture=tex,sampler_type=sampler))
        if suffix=='Color': connect(sample,'RGB',u.MaterialProperty.MP_BASE_COLOR)
        elif suffix=='Normal': connect(sample,'RGB',u.MaterialProperty.MP_NORMAL)
        else:
            connect(sample,'R',u.MaterialProperty.MP_AMBIENT_OCCLUSION)
            connect(sample,'G',u.MaterialProperty.MP_ROUGHNESS)
            connect(sample,'B',u.MaterialProperty.MP_METALLIC)
    library.save_loaded_asset(mat)
    return mat


materials = {name:surface(name) for name in ('FieldSpade','OakBarrel','DryHay')}
fibres = material('FieldFibres')
fiber_color=edit.create_material_expression(fibres,u.MaterialExpressionVertexColor,-240,0)
connect(fiber_color,'',u.MaterialProperty.MP_BASE_COLOR)
rough=edit.create_material_expression(fibres,u.MaterialExpressionConstant,-240,110)
rough.set_editor_property('r',.91)
connect(rough,'',u.MaterialProperty.MP_ROUGHNESS)
library.save_loaded_asset(fibres)
materials['FieldFibres']=fibres
for name in ('SM_FieldSpade','SM_OakBarrel','SM_BoundHay'):
    mesh=import_asset(name+'.glb', ROOT)
    slots=mesh.get_editor_property('static_materials')
    for index,slot in enumerate(slots):
        material_name=str(slot.material_slot_name)
        mesh.set_material(index,materials[material_name])
    if name != 'SM_FieldSpade':
        subsystem = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
        subsystem.remove_collisions(mesh)
        shape = u.ScriptCollisionShapeType.NDOP18 if name == 'SM_OakBarrel' else u.ScriptCollisionShapeType.BOX
        if subsystem.add_simple_collisions(mesh, shape) < 0:
            raise RuntimeError('Missing collision for ' + name)
    library.save_loaded_asset(mesh)
    u.log('COTTAGE YARD saved '+mesh.get_path_name()+' '+str(mesh.get_bounds())+
          ' triangles='+str(mesh.get_num_triangles(0))+' slots='+str([str(s.material_slot_name) for s in slots]))

# Interchange creates simple materials for the glTF slot names when these assets are
# first imported. The authored PBR materials now occupy those slots.
for name,slots in [('SM_FieldSpade',['FieldSpade']),('SM_OakBarrel',['OakBarrel']),
                   ('SM_BoundHay',['DryHay','FieldFibres'])]:
    for slot in slots:
        generated=f'{ROOT}/{name}/Materials/{slot}'
        if library.does_asset_exist(generated) and not library.find_package_referencers_for_asset(generated,True):
            library.delete_asset(generated)
