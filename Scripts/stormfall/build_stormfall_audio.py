"""Build original close lightning, falling fir and cave foley as spatial mono SoundWaves."""
import array
import math
import random
import wave
from pathlib import Path
import unreal as u

RATE=32000
DEST='/Game/NightOfTheWolves/Stormfall'
cache=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_intermediate_dir()))/'StormfallFoley'
cache.mkdir(parents=True,exist_ok=True)
u.EditorAssetLibrary.make_directory(DEST)


def sample(name,length,loop=False):
    r=random.Random(67010+len(name))
    out=[0.]*round(RATE*length)
    slow=mid=body=0.
    for i in range(len(out)):
        t=i/RATE; n=r.uniform(-1,1)
        slow+=.012*(n-slow); mid+=.12*(n-mid); body+=.035*(slow-body)
        if name=='ThunderRoll':
            # Three pressure fronts, followed by a reflection off the far valley.
            direct=max(0,1-t/.075)**2*(.42*n+1.32*mid)+.74*mid*math.exp(-8*t)
            rolling=(3.5*slow+4.7*body)*sum(g*math.exp(-abs(t-start)*decay) for start,g,decay in [(.18,1.0,3.2),(.72,.88,2.9),(1.35,.57,2.6),(2.24,.48,2.4),(3.25,.22,1.8)])
            out[i]=direct+rolling
        elif name=='CrownImpact':
            out[i]=1.23*mid*math.exp(-4*t)+3.*slow*math.exp(-2.4*t)+.30*math.sin(6.283*(79*t-15*t*t))*math.exp(-8*t)
        elif name=='FirBreak':
            out[i]=.16*(mid-slow)*math.sin(12*t)**2*math.exp(-.9*t)
        elif name=='PitchSnap':
            out[i]=(.42*mid+1.4*slow)*(.6+.13*math.sin(4.6*t)+.17*math.sin(8.9*t))*.48
        elif name=='RootsCatching':
            out[i]=(.31*n+.67*mid)*math.sin(math.pi*t/length)**2*(.25+.16*math.cos(63*t)+.13*math.sin(108*t))
    def hit(at,g,dur,low,bright):
        a=int(at*RATE); prev=0
        for j in range(round(dur*RATE)):
            age=j/RATE
            n=r.uniform(-1,1);prev+=bright*(n-prev)
            v=(.54*prev+.39*math.sin(6.283*low*age)+.12*math.sin(6.283*low*2.13*age))*math.exp(-age*6/dur)
            out[(a+j)%len(out)]+=g*min(1,age*1800)*v
    if name=='FirBreak':
        for at in [.01,.034,.11,.19,.233,.31,.39,.49,.57,.67,.78,.84,1.02,1.19]:
            hit(at,r.uniform(.53,1.1),r.uniform(.08,.21),r.uniform(140,410),r.uniform(.47,.92))
    elif name=='CrownImpact':
        for k in range(32):
            at=r.random()**1.8*1.34; hit(at,r.uniform(.21,.63)*math.exp(-at),.085+r.random()*.13,r.uniform(89,880),.63)
    elif name in ('PitchSnap','RootsCatching'):
        for k in range(int(length*13)):
            hit(r.uniform(.018,length-.09),r.uniform(.05,.56),r.uniform(.012,.072),r.uniform(620,1680),.87)
    elif name=='CaveSeep':
        for at in [.31,1.82,3.91,6.70,7.53]:
            pitch=r.uniform(730,1410)
            for dt,g in [(0,.85),(.075,.22),(.132,.1)]:
                hit(at+dt,g,.13,pitch,.18)
    peak=max(abs(v) for v in out)
    with wave.open(str(cache/(name+'.wav')),'wb') as w:
        w.setparams((1,2,RATE,0,'NONE','uncompressed'))
        w.writeframes(array.array('h',(round(26000*v/peak) for v in out)).tobytes())
    task=u.AssetImportTask()
    task.set_editor_properties(dict(filename=str(cache/(name+'.wav')),destination_path=DEST,automated=True,replace_existing=True,save=True))
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset=u.load_asset(DEST+'/'+name)
    if not asset or not task.imported_object_paths:raise RuntimeError('Missing '+name)
    asset.set_editor_properties(dict(looping=loop,compression_quality=88))
    u.EditorAssetLibrary.save_loaded_asset(asset,False)


for name,duration,loop in [('ThunderRoll',4.6,False),('FirBreak',1.55,False),('CrownImpact',2.40,False),
                            ('PitchSnap',4.8,True),('RootsCatching',1.75,False),('CaveSeep',9.8,True)]:
    sample(name,duration,loop)
u.log('STORMFALL FOLEY SAVED')
