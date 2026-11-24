"""Author the still cave pool and two original sounds of the old survey equipment."""
import array
import math
import random
import wave
from pathlib import Path
import unreal as u

DEST = "/Game/NightOfTheWolves/HighWater"
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
edit = u.MaterialEditingLibrary
assets.make_directory(DEST)


def pool():
    path = DEST + '/M_BlackPool'
    if assets.does_asset_exist(path):
        return
    mat = tools.create_asset('M_BlackPool', DEST, u.Material, u.MaterialFactoryNew())
    w = edit.create_material_expression(mat, u.MaterialExpressionWorldPosition, -720, 0)
    t = edit.create_material_expression(mat, u.MaterialExpressionTime, -720, 150)
    # Two scales of capillary waves and irregular circles from the spring.
    for prop, hlsl, kind, y in [
        (u.MaterialProperty.MP_NORMAL, """
            float2 p = P.xy;
            float2 v = .10*float2(cos(p.x*.036 + p.y*.012 + T*1.7),
                                  sin(p.y*.047 + p.x*.021 - T*1.3));
            float2 d = p - float2(15788, 2370);
            v += .17*normalize(d + .001)*cos(length(d)*.17 - T*6.1)*exp(-length(d)/155.);
            v += .026*float2(sin(p.x*.141 - T*2.2),cos(p.y*.121 + T));
            return normalize(float3(v,1));
        """, u.CustomMaterialOutputType.CMOT_FLOAT3, 40),
        (u.MaterialProperty.MP_BASE_COLOR, """
            float q=.5+.5*cos(P.x*.013+P.y*.021+T*.34);
            return lerp(float3(.009,.021,.020),float3(.025,.044,.046),q);
        """, u.CustomMaterialOutputType.CMOT_FLOAT3, 220),
    ]:
        node = edit.create_material_expression(mat, u.MaterialExpressionCustom, -300, y)
        inputs = []
        for key in ['P','T']:
            inp=u.CustomInput()
            inp.set_editor_property('input_name',key)
            inputs.append(inp)
        node.set_editor_properties(dict(code=hlsl,inputs=inputs,output_type=kind))
        for val,key in [(w,'P'),(t,'T')]:
            edit.connect_material_expressions(val,'',node,key)
        edit.connect_material_property(node,'',prop)
    rough = edit.create_material_expression(mat, u.MaterialExpressionConstant, -60, 400)
    rough.set_editor_property('r',.16)
    edit.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
    assets.save_loaded_asset(mat,False)


def foley(name, duration):
    rate=32000
    samples = [0.] * int(rate*duration)
    rng=random.Random(111 if name=='FloatTap' else 404)
    def knock(at, frequency, strength, decay):
        start=int(rate*at)
        filtered=0.
        for i in range(min(int(decay*rate),len(samples)-start)):
            t=i/rate
            filtered += .44*(rng.uniform(-1,1)-filtered)
            body=sum(a*math.sin(math.tau*frequency*q*t)*math.exp(-t*k/decay)
                     for a,q,k in [(.45,1,7),(.28,2.37,10),(.13,3.24,14)])
            samples[start+i] += strength*min(1.,t*900)*(body + filtered*.65*math.exp(-t*20/decay))
    if name=='FloatTap':
        # The corner lifts against a guide, settles, then returns a short hollow echo.
        for at,f,g,d in [(.008,154,1.,.40),(.077,267,.33,.27),(.129,145,.19,.42),(.224,169,.085,.38)]:
            knock(at,f,g,d)
    elif name=='PoolSplash':
        bass=mid=0.
        for i in range(len(samples)):
            t=i/rate
            noise=rng.uniform(-1.,1.)
            bass+=.019*(noise-bass)
            mid+=.26*(noise-mid)
            samples[i]=min(t*95,1.)*((2.4*bass+.33*mid)*math.exp(-7*t)+
                       .39*(noise-mid)*math.exp(-18*abs(t-.11)))
        for j in range(22):
            knock(rng.uniform(.08,.45),rng.uniform(390,1190),rng.uniform(.03,.12),.065)
    else:
        filtered=0.; phase=0.
        for i in range(len(samples)):
            t=i/rate
            filtered+=.13*(rng.uniform(-1.,1.)-filtered)
            phase+=math.tau*(111+26*math.sin(t*5.2)+9*math.sin(t*25))/rate
            envelope=math.sin(math.pi*t/duration)**1.6
            samples[i]=envelope*(.23*filtered+.076*math.sin(phase)+.027*math.sin(phase*2.02))*(.54+.46*math.sin(47*t)**2)
        for j in range(14):
            at=.07+j*.07+rng.uniform(-.02,.02)
            knock(at,rng.uniform(86,225),rng.uniform(.08,.29),rng.uniform(.08,.22))
    cache=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_intermediate_dir()))/'SurveyFoley'
    cache.mkdir(parents=True,exist_ok=True)
    file=cache/(name+'.wav')
    peak=max(abs(x) for x in samples)
    with wave.open(str(file),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','uncompressed'))
        w.writeframes(array.array('h',(round(x/peak*24000) for x in samples)).tobytes())
    task=u.AssetImportTask()
    task.set_editor_properties(dict(filename=str(file),destination_path=DEST,automated=True,replace_existing=True,save=True))
    tools.import_asset_tasks([task])
    obj=u.load_asset(DEST+'/'+name)
    if not obj or not task.imported_object_paths:
        raise RuntimeError('Sound import failed: '+name)
    obj.set_editor_properties(dict(looping=False,compression_quality=90))
    assets.save_loaded_asset(obj,False)

pool()
foley('FloatTap',.67)
foley('CedarGroan',1.2)
foley('PoolSplash',.62)
u.log('HIGH WATER POOL AND FOLEY SAVED')
