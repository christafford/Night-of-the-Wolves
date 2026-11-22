"""Import the source wolf and bind its coat, masked guard hairs, facial markings and wet features.

Run Scripts/wolves/author_timber_wolf.py with Python 3 before this editor commandlet.
"""
from pathlib import Path
import unreal as u

root = "/Game/NightOfTheWolves/Wolves"
source_dir = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())) / "Art/Source/FirstLeg/Wolves"
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
edit = u.MaterialEditingLibrary
mesh_path = root + "/SK_TimberWolf"
source = source_dir / "SK_TimberWolf.glb"
mesh_only = "-WolfMeshOnly" in u.SystemLibrary.get_command_line().split()

def import_file(path, directory):
    task=u.AssetImportTask()
    task.set_editor_properties(dict(filename=str(path),destination_path=directory,replace_existing=True,automated=True,save=True))
    tools.import_asset_tasks([task])
    return task.get_objects()

if assets.does_asset_exist(mesh_path):
    mesh=assets.load_asset(mesh_path)
    # The source was originally imported to an Interchange subdirectory, then
    # moved to its runtime path. Store the resolved path relative to that package.
    mesh.get_editor_property("asset_import_data").scripted_add_filename(str(source),0,"")
    params=u.ImportAssetParameters()
    params.set_editor_property("is_automated",True)
    result=u.InterchangeManager.get_interchange_manager_scripted().reimport_asset(mesh,params)
    if not result: raise RuntimeError("Unable to reimport "+str(source))
else:
    imported=[a for a in import_file(source,root) if isinstance(a,u.SkeletalMesh)]
    if len(imported)!=1: raise RuntimeError("Expected one wolf")
    mesh=imported[0]
    if mesh.get_path_name()!=mesh_path+".SK_TimberWolf":
        if not assets.rename_asset(mesh.get_path_name(),mesh_path): raise RuntimeError("Could not move wolf to runtime path")
    mesh=assets.load_asset(mesh_path)

def texture(name,normal=False,masked=False):
    directory=root+"/Textures"
    import_file(source_dir/(name+".png"),directory)
    tex=assets.load_asset(directory+"/"+name)
    if not tex: raise RuntimeError("Could not import "+name)
    if normal:
        tex.set_editor_properties(dict(compression_settings=u.TextureCompressionSettings.TC_NORMALMAP,srgb=False))
    if masked:
        tex.set_editor_properties(dict(srgb=True,do_scale_mips_for_alpha_coverage=True,
                    alpha_coverage_thresholds=u.Vector4(0,0,0,.29)))
    if not assets.save_loaded_asset(tex,False): raise RuntimeError("Could not save "+name)
    return tex

if not mesh_only:
    undercoat=texture("TimberUndercoat")
    guard=texture("TimberGuardHair",masked=True)
    markings=texture("TimberFaceMarkings")
    normal=texture("TimberUndercoatNormal",normal=True)
    leather=texture("TimberNoseLeather",normal=True)

remaining_nodes = {}

def material(name,masked=False):
    m=assets.load_asset(root+"/M_"+name) if assets.does_asset_exist(root+"/M_"+name) else tools.create_asset("M_"+name,root,u.Material,u.MaterialFactoryNew())
    m.set_editor_property("used_with_skeletal_mesh",True)
    m.set_editor_property("two_sided",masked)
    m.set_editor_property("blend_mode",u.BlendMode.BLEND_MASKED if masked else u.BlendMode.BLEND_OPAQUE)
    if masked: m.set_editor_property("opacity_mask_clip_value",.29)
    # Runtime CDOs load this mesh and its material nodes before editor startup.
    # Reuse the saved graph in place; UE may have rooted these expressions at that point.
    remaining_nodes.clear()
    for expr in edit.get_material_expressions(m):
        x,y=edit.get_material_expression_node_position(expr)
        remaining_nodes.setdefault((expr.get_class().get_name(),x,y),[]).append(expr)
    return m

def node(m,cls,x,y):
    saved=remaining_nodes.get((cls.static_class().get_name(),x,y))
    if saved: return saved.pop(0)
    return edit.create_material_expression(m,cls,x,y)

def parameter(m,name,default,x,y):
    a=node(m,u.MaterialExpressionScalarParameter,x,y)
    a.set_editor_properties(dict(parameter_name=name,default_value=default))
    return a

def sample(m,tex,x,y,index=0,normal_map=False):
    a=node(m,u.MaterialExpressionTextureSample,x,y)
    a.set_editor_properties(dict(texture=tex,const_coordinate=index,
                                sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal_map else u.MaterialSamplerType.SAMPLERTYPE_COLOR))
    return a

def bind(expr,output,target,input_name):
    if not edit.connect_material_expressions(expr,output,target,input_name):
        raise RuntimeError(f"Cannot connect {output} to {input_name}")

def pin(expr,output,property_):
    if not edit.connect_material_property(expr,output,property_):
        raise RuntimeError(f"Cannot connect {output} to {property_}")

def custom(m,description,source,connections):
    a=node(m,u.MaterialExpressionCustom,-80,20)
    labels=[name for name,_,_ in connections]
    existing=[str(pin.get_editor_property("input_name")) for pin in a.get_editor_property("inputs")]
    if existing != labels:
        inputs=[]
        for name in labels:
            item=u.CustomInput();item.set_editor_property("input_name",name);inputs.append(item)
        a.set_editor_property("inputs",inputs)
    a.set_editor_properties(dict(description=description,code=source,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3))
    for name,expr,output in connections: bind(expr,output,a,name)
    return a

def roughness(m,default,spec):
    pin(parameter(m,"Roughness",default,-280,450),"",u.MaterialProperty.MP_ROUGHNESS)
    pin(parameter(m,"Specular",spec,-280,525),"",u.MaterialProperty.MP_SPECULAR)

materials={}
for name in ("TimberCoat","TimberFace","TimberRuff","TimberWet","TimberEye"):
    if mesh_only:
        materials[name]=assets.load_asset(root+"/M_"+name)
        if not materials[name]: raise RuntimeError("Missing saved material "+name)
        continue
    m=material(name,name=="TimberRuff")
    paint=node(m,u.MaterialExpressionVertexColor,-825,12)
    if name in ("TimberCoat","TimberFace","TimberRuff"):
        tone=parameter(m,"CoatValue",1.,-825,100)
    if name in ("TimberCoat","TimberFace"):
        fur=sample(m,undercoat,-825,230)
        pins=[("Fur",paint,""),("Value",tone,""),("Fibre",fur,"RGB")]
        code="""float l = dot(Fibre,float3(.2126,.7152,.0722));
float factor = clamp(.52 + l*2.1, .48, 1.7);
float3 coat = Fur*factor;"""
        if name=="TimberFace":
            plate=sample(m,markings,-825,490,1)
            pins += [("Pattern",plate,"RGB"),("Amount",paint,"A")]
            code += """
float tone = dot(Pattern,float3(.2126,.7152,.0722));
float3 silver = lerp(float3(.035,.041,.047),float3(.72,.74,.74),pow(saturate(tone),.75));
coat = lerp(coat, silver*(.92+.12*factor), Amount*.98);"""
        c=custom(m,"Agouti bands and snout pigment",code+"\nreturn coat*Value;",pins)
        pin(c,"",u.MaterialProperty.MP_BASE_COLOR)
        pin(sample(m,normal,-310,260,normal_map=True),"RGB",u.MaterialProperty.MP_NORMAL)
        roughness(m,.78,.31)
    elif name=="TimberRuff":
        locks=sample(m,guard,-825,218)
        c=custom(m,"Silver-banded guard hairs","return Fur * Value * (.64 + 1.3*Lock.r);",
                 [("Fur",paint,""),("Value",tone,""),("Lock",locks,"RGB")])
        pin(c,"",u.MaterialProperty.MP_BASE_COLOR)
        pin(locks,"A",u.MaterialProperty.MP_OPACITY_MASK)
        roughness(m,.75,.32)
    elif name=="TimberWet":
        pin(paint,"",u.MaterialProperty.MP_BASE_COLOR)
        grain=sample(m,leather,-800,240,normal_map=True)
        surface=custom(m,"Leathery nose relief","return normalize(lerp(Grain,float3(0,0,1),Other));",
                       [("Grain",grain,"RGB"),("Other",paint,"A")])
        pin(surface,"",u.MaterialProperty.MP_NORMAL)
        moisture=custom(m,"Damp lip and subtly rough nose","return lerp(.55,.33,Other);",[("Other",paint,"A")])
        pin(moisture,"",u.MaterialProperty.MP_ROUGHNESS)
        pin(parameter(m,"Specular",.39,-280,525),"",u.MaterialProperty.MP_SPECULAR)
    else:
        pin(paint,"",u.MaterialProperty.MP_BASE_COLOR)
        shine=custom(m,"Retinal return and corneal glint","return Iris * ReturnStrength;",
                     [("Iris",paint,""),("ReturnStrength",parameter(m,"EyeReturn",.12,-825,222),"")])
        pin(shine,"",u.MaterialProperty.MP_EMISSIVE_COLOR)
        roughness(m,.18,.5)
    edit.recompile_material(m)
    if not assets.save_loaded_asset(m,False): raise RuntimeError("Could not save material "+name)
    materials[name]=m

slots=mesh.get_editor_property("materials")
for i,item in enumerate(slots):
    name=str(item.material_slot_name)
    if name not in materials: raise RuntimeError("Unexpected material slot "+name+" in "+str([str(s.material_slot_name) for s in slots]))
    slot=item.copy();slot.material_interface=materials[name];slots[i]=slot
mesh.set_editor_property("materials",slots)
if not assets.save_loaded_asset(mesh,False): raise RuntimeError("Could not save wolf mesh")
rest=u.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property("skeleton"))
for joint,expected in (("Root",(0,0,0)),("Skull",(44,0,76)),("ForeLPaw",(25,-13.5,5)),("HindRPaw",(-40,13.5,5))):
    point=rest.get_bone_pose(joint,u.AnimPoseSpaces.WORLD).translation
    if (point-u.Vector(*expected)).length()>.2:
        raise RuntimeError("Unexpected wolf joint basis "+joint+" "+str(point))
u.log("WOLF APPEARANCE SAVED "+mesh.get_path_name()+" "+str([str(s.material_slot_name) for s in slots])+" bounds "+str(mesh.get_bounds()))
