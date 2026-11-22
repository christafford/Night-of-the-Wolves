"""Make the first leg's original wolf silhouette, creek surface and spatial sound cues in Unreal Editor."""
import array
import base64
import json
import math
import os
import random
import struct
import tempfile
import wave

import unreal

DEST = "/Game/NightOfTheWolves/Crossing"
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

def import_file(filename):
    name = os.path.splitext(os.path.basename(filename))[0]
    path = DEST + "/" + name
    if filename.endswith(".gltf"):
        path += "/StaticMeshes/" + name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        task = unreal.AssetImportTask()
        task.set_editor_properties(dict(filename=filename, destination_path=DEST, automated=True, save=True))
        TOOLS.import_asset_tasks([task])
    obj = unreal.load_asset(path)
    if not obj:
        raise RuntimeError("Missing imported asset " + path)
    return obj


def wolf_mesh(folder):
    # Model in metres in glTF space (Y up, looking down local +X).
    vertices, indices = [], []
    def body(rings, sides=10):
        start = len(vertices)
        for x, y, z, width, height in rings:
            for i in range(sides):
                angle = 2 * math.pi * i / sides
                vertices.append((x, z + height * math.cos(angle), y + width * math.sin(angle)))
        for j in range(len(rings)-1):
            for i in range(sides):
                a = start + j*sides + i
                b = start + j*sides + (i+1) % sides
                indices.extend([a, b, a+sides, b, b+sides, a+sides])
        for j in range(1,sides-1):
            indices.extend([start, start+j+1, start+j])
            end = start + (len(rings)-1)*sides
            indices.extend([end, end+j, end+j+1])
    body([(-.66,0,.72,.07,.09),(-.54,0,.73,.17,.24),(-.30,0,.77,.18,.22),(.07,0,.79,.20,.29),
          (.25,0,.82,.21,.28),(.40,0,.90,.17,.23),(.48,0,.98,.14,.16),
          (.60,0,1.00,.13,.14),(.68,0,.965,.10,.08),(.82,0,.955,.052,.053)])
    body([(-1.11,0,.32,.032,.045),(-.98,0,.39,.106,.11),(-.81,.01,.50,.13,.13),(-.69,.02,.65,.11,.13),(-.55,0,.78,.08,.09)])
    for sign in (-1,1):
        s = .12*sign
        # Four legs with distinct elbow and hock silhouettes.
        for points in [[(.20,.72,.094),(.16,.42,.064),(.20,.19,.038),(.21,.07,.036),(.27,.041,.055)],
                       [(-.44,.69,.14),(-.33,.44,.095),(-.55,.23,.048),(-.54,.075,.036),(-.47,.04,.054)]]:
            # Ring axis follows each limb's height.
            start = len(vertices)
            for x,z,r in points:
                for i in range(8):
                    a=2*math.pi*i/8
                    vertices.append((x+math.cos(a)*r + .045*sign, z, s+math.sin(a)*r*.74))
            for j in range(len(points)-1):
                for i in range(8):
                    a=start+j*8+i; b=start+j*8+(i+1)%8
                    indices.extend([a,a+8,b,b,a+8,b+8])
        start=len(vertices)
        vertices.extend([(.46,1.08,s-.072),(.63,1.07,s),(.48,1.07,s+.075),(.48,1.28,s*1.20)])
        indices.extend(start+i for i in [0,1,3,1,2,3,2,0,3,0,2,1])
    # Face normals and flat shading give the distant pack a moving silhouette in moonlight.
    verts=[]; normals=[]
    for i in range(0,len(indices),3):
        a,b,c=[vertices[p] for p in indices[i:i+3]]
        u=[b[k]-a[k] for k in range(3)]; v=[c[k]-a[k] for k in range(3)]
        n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
        size=max(1e-8, math.sqrt(sum(t*t for t in n)))
        normal=[t/size for t in n]
        verts.extend([a,b,c]); normals.extend([normal]*3)
    def pack(values):
        return b''.join(struct.pack('<3f',*p) for p in values)
    positions=pack(verts); data=positions+pack(normals)
    filename=os.path.join(folder,'SM_LeadWolf.gltf')
    obj={"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0,"name":"SM_LeadWolf"}],
         "meshes":[{"name":"SM_LeadWolf","primitives":[{"attributes":{"POSITION":0,"NORMAL":1},"material":0}]}],
         "materials":[{"name":"WolfSlate","doubleSided":True,"pbrMetallicRoughness":{"baseColorFactor":[.13,.15,.17,1],"metallicFactor":0,"roughnessFactor":.96}}],
         "buffers":[{"byteLength":len(data),"uri":"data:application/octet-stream;base64,"+base64.b64encode(data).decode()}],
         "bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":len(positions)},{"buffer":0,"byteOffset":len(positions),"byteLength":len(positions)}],
         "accessors":[{"bufferView":0,"componentType":5126,"count":len(verts),"type":"VEC3","min":[min(p[i] for p in verts) for i in range(3)],"max":[max(p[i] for p in verts) for i in range(3)]},
                      {"bufferView":1,"componentType":5126,"count":len(verts),"type":"VEC3"}]}
    with open(filename,'w') as f: json.dump(obj,f)
    import_file(filename)


def sound(folder,name,duration):
    rng=random.Random(918)
    rate=22050
    data=array.array('h')
    slow=mid=previous=0
    for i in range(int(duration*rate)):
        t=i/rate; white=rng.uniform(-1,1)
        slow=slow*.975 + white*.025
        mid=mid*.70+white*.30
        if name=='CreekCurrent':
            sample=(mid*.36+slow*2.1 + (white-previous)*.028)*(.80+.15*math.sin(t*math.tau/duration*3))
        elif name=='PouchCoins':
            sample=sum((math.sin(f*t*math.tau)+.45*math.sin(f*2.37*t*math.tau))*.14*math.exp(-12*(t-d))
                       for f,d in [(1570,0),(2300,.08),(1850,.17),(2720,.26)] if t>=d)
        else:
            sample=(white*.55+mid*1.1)*math.exp(-26*t)+slow*2.6*math.exp(-5*t)
        previous=white
        data.append(int(32767*max(-.96,min(.96,sample))))
    path=os.path.join(folder,name+'.wav')
    with wave.open(path,'wb') as f:
        f.setparams((1,2,rate,0,'NONE','not compressed')); f.writeframes(data.tobytes())
    obj=import_file(path)
    if name=='CreekCurrent':
        obj.set_editor_property('looping',True)
        unreal.EditorAssetLibrary.save_loaded_asset(obj)


def water_material():
    path=DEST+'/M_RunningWater'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return
    m=TOOLS.create_asset('M_RunningWater',DEST,unreal.Material,unreal.MaterialFactoryNew())
    edit=unreal.MaterialEditingLibrary
    def expr(c,x,y): return edit.create_material_expression(m,c,x,y)
    def link(a,b,p): edit.connect_material_expressions(a,'',b,p)
    uv=expr(unreal.MaterialExpressionTextureCoordinate,-900,0)
    uv.set_editor_properties(dict(u_tiling=7.0,v_tiling=2.0))
    pan=expr(unreal.MaterialExpressionPanner,-720,0)
    pan.set_editor_properties(dict(speed_x=.07,speed_y=-.42)); link(uv,pan,'Coordinate')
    sample=expr(unreal.MaterialExpressionTextureSample,-500,0)
    sample.set_editor_property('texture',unreal.load_asset('/Game/NightOfTheWolves/Presentation/grey_stone_path'))
    link(pan,sample,'UVs')
    fresnel=expr(unreal.MaterialExpressionFresnel,-490,-270)
    mul=expr(unreal.MaterialExpressionMultiply,-280,0)
    edit.connect_material_expressions(sample,'R',mul,'A'); link(fresnel,mul,'B')
    lerp=expr(unreal.MaterialExpressionLinearInterpolate,-70,0)
    for pin,col,y in [('A',(.008,.035,.044),250),('B',(.30,.56,.62),440)]:
        color=expr(unreal.MaterialExpressionConstant3Vector,-450,y)
        color.set_editor_property('constant',unreal.LinearColor(*col,1));link(color,lerp,pin)
    link(mul,lerp,'Alpha')
    edit.connect_material_property(lerp,'',unreal.MaterialProperty.MP_BASE_COLOR)
    glow=expr(unreal.MaterialExpressionMultiply,120,240)
    glow.set_editor_property('const_b',.22)
    link(lerp,glow,'A');edit.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rough=expr(unreal.MaterialExpressionConstant,0,400); rough.set_editor_property('r',.22)
    edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.EditorAssetLibrary.save_loaded_asset(m)


with tempfile.TemporaryDirectory() as temp:
    wolf_mesh(temp)
    for name,duration in [('CreekCurrent',6),('PouchCoins',1),('DistantShot',1.8)]:
        sound(temp,name,duration)
    water_material()
    material = unreal.load_asset(DEST+'/M_RunningWater')
    material.set_editor_property('used_with_instanced_static_meshes', True)
    material.set_editor_property('used_with_nanite', True)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Could not save creek material permutations")
unreal.log('CROSSING_ASSETS_READY')
