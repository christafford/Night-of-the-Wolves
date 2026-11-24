"""Bake the broken basalt bed, cedar roots, lightning channel and fire cards with Geometry Script."""
import math
import random
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "cabin"))
from carpentry_mesh import Joinery, V
from build_log_cabin import bake
from finish_cottage_supplies import tube

DEST = "/Game/NightOfTheWolves/Stormfall"
CASC = "/Game/NightOfTheWolves/Cascades/"
CARP = "/Game/NightOfTheWolves/FirstLeg/Carpentry/"
U = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary
U.make_directory(DEST)
rng = random.Random(14051)
STONE, DAMP, TREAD, BARK, HEART, BLACK, ARC, FIRE = range(8)


def shader(name, flame=False, bolt=False, char=False):
    path = DEST + "/" + name
    if U.does_asset_exist(path):
        return U.load_asset(path)
    mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name, DEST, u.Material, u.MaterialFactoryNew())
    mat.set_editor_property("two_sided", True)
    def node(kind, x, y):
        return edit.create_material_expression(mat, kind, x, y)
    def output(pin, expr):
        if not edit.connect_material_property(expr, "", pin):
            raise RuntimeError("Unconnected material " + name)
    def custom(code, inputs, kind=u.CustomMaterialOutputType.CMOT_FLOAT3, y=0):
        c = node(u.MaterialExpressionCustom, -110, y)
        pins = []
        for key, value in inputs:
            i = u.CustomInput()
            i.set_editor_property("input_name", key)
            pins.append(i)
        c.set_editor_properties(dict(inputs=pins, code=code, output_type=kind))
        for key, value in inputs:
            edit.connect_material_expressions(value, "", c, key)
        return c
    if flame or bolt:
        mat.set_editor_property("shading_model", u.MaterialShadingModel.MSM_UNLIT)
    if bolt:
        e = node(u.MaterialExpressionConstant3Vector, -200, 0)
        e.set_editor_property("constant", u.LinearColor(18.,29.,47.,1))
        output(u.MaterialProperty.MP_EMISSIVE_COLOR, e)
    elif char:
        uv = node(u.MaterialExpressionTextureCoordinate,-650,0)
        v = custom("float g=frac(17.*UV.x+.14*sin(UV.y*16.)+.08*sin(UV.y*38.));return lerp(float3(.009,.006,.004),float3(.095,.054,.028),smoothstep(.30,.7,g));",[("UV",uv)])
        output(u.MaterialProperty.MP_BASE_COLOR,v)
    else:
        mat.set_editor_property("blend_mode",u.BlendMode.BLEND_MASKED)
        mat.set_editor_property("opacity_mask_clip_value",.26)
        uv = node(u.MaterialExpressionTextureCoordinate, -650, 0)
        time = node(u.MaterialExpressionTime, -650, 210)
        inputs=[("UV",uv),("T",time)]
        common = """float h=saturate(UV.y);float x=UV.x*2-1;
float wave=.14*sin(8.*h-5.4*T)+.11*sin(14.*h-8.1*T+UV.x*2.)+.10*sin(T*3.+h*3.);
float body=(1.-h)*(.80+.11*sin(T*6.8+h*13.))-.10*abs(sin(16.*h-5.*T));
float q=saturate((body-abs(x-wave*(.15+h)))*3.8);
"""
        output(u.MaterialProperty.MP_OPACITY_MASK,custom(common+"return q*smoothstep(0.,.055,h);",inputs,u.CustomMaterialOutputType.CMOT_FLOAT1,190))
        output(u.MaterialProperty.MP_EMISSIVE_COLOR,custom(common+"return lerp(float3(4.3,.31,.013),float3(12.,5.7,.97),pow(q,2.)*(1.-.85*h));",inputs))
    U.save_loaded_asset(mat,False)
    return mat


palette = [u.load_asset(CASC+'M_StreamStone'),u.load_asset(CASC+'M_CreekBank'),u.load_asset(CASC+'M_DampTread'),
           u.load_asset(CASC+'M_fir_tree_01_Bark'),u.load_asset(CARP+'M_CutEnds'),
           shader('M_SplitChar',char=True),shader('M_SkyChannel',bolt=True),shader('M_PitchFlame',flame=True)]
if not all(palette):
    raise RuntimeError("Missing stormfall scan")


def face(s, slot, pts, look, tex=110):
    a,b,c=pts[:3]
    n=(b-a).cross(c-a).normal()
    if n.dot(look)<0: n=n*-1.
    if abs(n.z)>.64: uv=[(p.x/tex,p.y/tex) for p in pts]
    elif abs(n.x)>abs(n.y): uv=[(p.y/tex,-p.z/tex) for p in pts]
    else: uv=[(p.x/tex,-p.z/tex) for p in pts]
    s.face(slot,pts,uv,n,rng.uniform(.84,1.0))


def crag(s, center, extent, slot=STONE, facets=8):
    # Sheared, split rock with a broad broken top and strongly angled bedding planes.
    cx,cy,cz=center; sx,sy,sz=extent
    phi=rng.random()*6.28; bias=rng.uniform(-.30,.30)
    rings=[]
    shapes=[rng.uniform(.85,1.15) for _ in range(facets)]
    for zmul,rad in [(-1,.67),(-.31,1.),(.42,.90),(1.,.52)]:
        ring=[]
        for j,k in enumerate(shapes):
            theta=phi+math.tau*j/facets
            ring.append(V(cx+sx*(math.cos(theta)*k*rad+zmul*bias),
                          cy+sy*(math.sin(theta)*k*rad+zmul*.12),cz+sz*(zmul+.14*math.cos(theta)+rng.uniform(-.07,.07))))
        rings.append(ring)
    for lo,hi in zip(rings,rings[1:]):
        for j,p in enumerate(lo):
            k=(j+1)%facets
            face(s,slot,[p,lo[k],hi[k],hi[j]],(p+lo[k])*.5-V(cx,cy,cz))
    face(s,slot,rings[-1],V(0,0,1))


def stitch(s, first, second):
    """Close two +Y arch rims, each running from its -X foot over the crown to its +X foot."""
    for j in range(len(first)):
        k = (j + 1) % len(first)
        for a, b, c in ((first[j], second[j], second[k]), (first[j], second[k], first[k])):
            face(s, STONE, [a, b, c], (b-a).cross(c-a), 153)


def cave(cleft_outlet=None):
    s=Joinery()
    # Angular asymmetric sections connect a broad vestibule to a lower cross-fissure.
    profile=[(1.,-.06),(.99,.31),(.89,.66),(.70,.90),(.24,1.),(-.30,.95),(-.77,.83),(-.96,.48),(-1.,.15),(-.97,-.06)]
    def bore(stations,sidepass):
        rings=[]
        for t,center,width,roof,floor in stations:
            row=[]
            for k,(f,h) in enumerate(profile):
                q=width*f+rng.uniform(-12,12)
                height=floor+roof*h+rng.uniform(-8,8)
                row.append(V(center+q,t,height) if sidepass else V(t,center+q,height))
            rings.append(row)
        for i,(a,b) in enumerate(zip(rings,rings[1:])):
            mid=(stations[i][0]+stations[i+1][0])*.5
            for j in range(len(profile)-1):
                if not sidepass and 332<mid<659 and j<3: continue
                pts=[a[j],b[j],b[j+1],a[j+1]]
                inside=V(stations[i][1],mid,90) if sidepass else V(mid,stations[i][1],90)
                face(s,STONE,pts,inside-(a[j]+b[j+1])*.5,153)
        return rings
    main=[(-82,-6,239,330,-3),(43,2,232,298,1),(180,-14,225,350,4),
          (332,-12,247,383,6),(440,-10,263,357,8),(555,-1,256,342,9),
          (659,4,233,325,10),(795,-24,204,278,13),(875,-12,133,195,14)]
    vestibule=bore(main,False)
    side=[(164,498,158,248,9),(227,501,145,253,11),(344,499,149,248,12),
          (462,503,163,261,14),(612,521,202,305,17),(755,543,222,338,20),(879,565,190,324,22),(939,560,147,237,23)]
    fissure=bore(side,True)
    # Stitched dirt tread follows foot collision. Its lip follows the old road grade.
    sections=[(-650,480,-45),(-470,440,-32),(-346,388,-20),(-170,311,-8),(0,258,0),
              (180,247,4),(330,257,6),(490,274,10),(655,252,11),(840,219,14)]
    rows=[]
    for x,w,z in sections:
        rows.append([V(x+(-18 if k in (0,6) else 0),(-1+k/3)*w,z+(abs(k-3)/3)**3*rng.uniform(2,11)) for k in range(7)])
    for a,b in zip(rows,rows[1:]):
        for j in range(6): face(s,TREAD if 1<=j<=4 else DAMP,[a[j],b[j],b[j+1],a[j+1]],V(0,0,1),158)
    for (y,c,w,h,z),(yy,cc,ww,hh,zz) in zip(side,side[1:]):
        face(s,DAMP,[V(c-w,y,z-.12),V(c+w,y,z-.12),V(cc+ww,yy,zz-.12),V(cc-ww,yy,zz-.12)],V(0,0,1),138)
    # Shattered ridge faces surround the open mouth; there is no masonry trim.
    ridge=[]
    for i,x in enumerate((-7,125,285,502,728,941,1100)):
        row=[]
        for k in range(15):
            y=(k-7)*205
            z=459+51*math.sin(.005*x+y*.002)+rng.uniform(-15,21)
            if i==0: z-=47
            if abs(y)>930: z-=min(180,(abs(y)-930)*.32)
            # Over the new ascending cleft the outside of the hill must remain
            # above the vault. Ease back into the old ridge outside its footprint.
            if y>820:
                blend=max(0.,min(1.,(x-72.)/204.,(981.-x)/205.,(y-820.)/160.))
                roof=359.+max(0.,y-910.)*.455
                z+=max(0.,roof-z)*blend
            row.append(V(x+(rng.uniform(-23,25) if i else 0),y,z))
        ridge.append(row)
    for lo,hi in zip(ridge,ridge[1:]):
        for j in range(len(lo)-1):
            face(s,DAMP,[lo[j],hi[j],hi[j+1],lo[j+1]],V(0,0,1),220)
    for sign in (-1,1):
        for i in range(10):
            yy=sign*(316+i*147); xx=rng.uniform(-37,33)+abs(yy)*.12
            width=rng.uniform(135,195)
            inner=xx+201
            inner_width=225
            if yy>845:
                # Keep these two courses on the *outside* of the passage. Their old
                # deep tongues crossed the camera path and, in places, Max's head.
                xx=min(166.,xx)
                width=min(103.,width)
                inner=min(224.,inner)
                inner_width=52.
            crag(s,(xx,yy,128),(width,rng.uniform(115,162),rng.uniform(156,248)))
            crag(s,(inner,yy,246),(inner_width,154,rng.uniform(172,231)))
        for x in range(121,935,130):
            crag(s,(x,sign*rng.uniform(327,358),178),(109,84,rng.uniform(105,172))) if sign<0 or x<303 or x>663 else None
    for i in range(8):
        y=-265+i*78
        crag(s,(rng.uniform(-80,10),y,343+37*(1-abs(y)/312)),(108,107,55+rng.random()*47))
    for i in range(21):
        y=rng.uniform(-190,192); z=rng.uniform(12,97)
        crag(s,(rng.uniform(747,887),y,z),(rng.uniform(58,123),rng.uniform(55,113),rng.uniform(53,103)))
    # Fallen side blocks leave the throat open to the rising cleft beyond CP6.
    for x in (371, 711):
        crag(s,(x,921,55),(48,67,66))
    # Mineral ledges trace the blind wall; keep the route and ankle clearance free.
    for x in range(10,760,72):
        crag(s,(x,-214+rng.uniform(-13,14),48),(55,36,rng.uniform(43,64)))
    for i in range(43):
        if i%3:
            x=rng.uniform(-300,745); y=rng.choice((-1,1))*rng.uniform(147,220)
            if 344<x<662 and y>0: continue
        else: x=rng.choice((371,629)); y=rng.uniform(290,790)
        q=rng.uniform(3,15);crag(s,(x,y,8),(q*1.6,q,q*.55),facets=5)
    # Seal the blind end behind the talus and stitch the actual split in the
    # vestibule wall to the side passage. The irregular lips are not coplanar.
    decor_state=rng.getstate()
    face(s,STONE,vestibule[-1],V(-1,0,0),153)
    for rings in (vestibule,fissure):
        for a,b in zip(rings,rings[1:]):
            face(s,DAMP,[a[0],a[-1],b[-1],b[0]],V(0,0,1),153)
    jamb = [vestibule[3][j] for j in range(4)] + [vestibule[i][3] for i in (4,5)] + [
        vestibule[6][j] for j in range(3,-1,-1)]
    stitch(s, jamb, list(reversed(fissure[2])))
    for i in (4,5):
        face(s,DAMP,[vestibule[3][0],vestibule[i][0],vestibule[i+1][0]],V(0,0,1),153)
    rng.setstate(decor_state)
    if cleft_outlet is not None:
        cleft_outlet.extend(reversed(fissure[-1]))
    return s


def roots(seed):
    r=random.Random(seed); s=Joinery()
    # The fan of fine rootlets twists and narrows continuously as it leaves the soil.
    for i in range(17):
        x=(i-8)*4.4+r.uniform(-3,3)
        height=r.uniform(142,238)
        phase=r.uniform(-math.pi,math.pi)
        bias=r.uniform(-44,47)
        weight=r.uniform(1.8,4.8)
        points=[]
        for j in range(13):
            t=j/12
            points.append(V(x+bias*t+12*math.sin(t*7+phase)+7*math.sin(t*15+phase*2),
                            7*math.sin(t*6+phase)+11*t*math.sin(t*9+phase),-height*t))
        for j in range(0,12,3):
            tube(s,BARK,points[j:j+4],weight*(1.-.77*j/12),False,6)
        for j in (3,6,9):
            base=points[j]; dx=r.choice((-1,1))*r.uniform(14,34); dy=r.uniform(-22,22)
            twig=[base+V(dx*t+4*math.sin(math.pi*t),dy*t,-(19+2*j)*t*t-12*t) for t in (0.,.27,.58,.82,1.)]
            tube(s,BARK,twig[:3],weight*.38,False,5)
            tube(s,BARK,twig[2:],weight*.18,False,4)
    return s


def lightning():
    s=Joinery();r=random.Random(633)
    path=[V(0,0,0)]
    for i in range(1,20):
        path.append(V(i*19+r.uniform(-75,75),-i*12+r.uniform(-65,65),i*184+r.uniform(-43,43)))
    tube(s,ARC,path,9.,False,6)
    for i in [3,5,7,10,13,16]:
        side=r.choice((-1,1));p=path[i]; strand=[p]
        for n in range(r.randint(4,8)):
            p=p+V(r.uniform(-145,60),side*r.uniform(65,152),r.uniform(-125,86))
            strand.append(p)
        tube(s,ARC,strand,3.7,False,5)
    return s


def flame():
    s=Joinery()
    for phi in (0.,math.pi*.40,math.pi*.77):
        right=V(math.cos(phi),math.sin(phi),0)
        up=V(0,0,1)
        edges=[]
        for t,w,lean in ((0,.58,0),(.13,.92,0),(.30,.69,.13),(.48,.51,-.08),(.68,.29,.10),(.86,.12,.29),(1,.012,.42)):
            center=up*(t*89)+right*(lean*15.)
            edges.append((center-right*(w*17.),center+right*(w*17.),t,w))
        for (a,b,t,w),(aa,bb,tt,ww) in zip(edges,edges[1:]):
            s.face(FIRE,[a,b,bb,aa],[(.5-w*.50,t),(.5+w*.50,t),(.5+ww*.50,tt),(.5-ww*.50,tt)],right.cross(up),1.)
    return s


def old_fir():
    s=Joinery(); r=random.Random(204)
    rings=[]; count=25
    for z,rad in ((-22,44),(0,46),(48,45),(119,40),(235,36),(372,31),(555,26),(715,20),(925,14),(1090,9),(1358,2)):
        row=[]
        for j in range(count):
            a=j*math.tau/count
            q=rad*(1.+.051*math.sin(a*7+z*.0007)+.046*math.cos(a*11))
            row.append(V(q*math.cos(a)+4*math.sin(z/420),q*math.sin(a)+z*.008,z))
        rings.append(row)
    for lo,hi in zip(rings,rings[1:]):
        for j in range(count):
            k=(j+1)%count; pts=[lo[j],lo[k],hi[k],hi[j]]
            mid=(lo[j]+hi[k])*.5
            s.face(BARK,pts,[(j/count*2.,lo[j].z/192),((j+1)/count*2.,lo[k].z/192),((j+1)/count*2.,hi[k].z/192),(j/count*2.,hi[j].z/192)],V(mid.x,mid.y,0).normal(),.85)
    face(s,HEART,rings[0],V(0,0,-1))
    for i in range(36):
        z=r.uniform(175,1060); a=i*.96 +r.uniform(-.39,.39)
        length=r.uniform(50,152)*(1. if z<800 else .84)
        radial=V(math.cos(a),math.sin(a),0)
        p=V(0,0,z);q=p+radial*(length*.52)+V(0,0,r.uniform(2,16))
        tip=q+radial*(length*.48)+V(0,0,r.uniform(7,39))
        tube(s,BARK,[p,q],r.uniform(6.,10.),False,8)
        tube(s,BARK,[q,tip],r.uniform(2.4,5.2),False,7)
        tube(s,BARK,[tip,tip+radial*(length*.16)+V(0,0,17)],1.7,False,5)
    return s


def shattered():
    s=Joinery()
    s.timber((0,0,-25),(0,0,77),49,slot=BARK,ends=HEART,r2=44)
    for i in range(23):
        p=i*math.tau/23; x=39*math.cos(p); y=37*math.sin(p); h=rng.uniform(97,193)
        base=V(x,y,66);a=V(-math.sin(p)*5,math.cos(p)*5,0)
        tip=V(x*.66+rng.uniform(-7,7),y*.81,h)
        face(s,HEART,[base-a,base+a,tip],V(-x,-y,5))
        face(s,BARK,[base+a,base-a,tip],V(x,y,2))
    return s


def burning_limb():
    s=Joinery()
    path=[V(0,1,337),V(4,53,376),V(8,96,398),V(15,146,417),V(14,182,431)]
    tube(s,BLACK,path,12.,False,10)
    tube(s,BARK,path[:3],15.,False,10)
    for i in range(4):
        p=path[min(4,i+1)];tube(s,BLACK,[p,p+V(0,20,20),p+V(8,42,41+i*5)],4.-i*.55,False,7)
    return s


if __name__ == "__main__":
    for name,mesh in [('SM_RivenBluff',cave()),('SM_LightningChannel',lightning()),('SM_PitchFlame',flame()),
                      ('SM_OldFirBole',old_fir()),('SM_SplitFirStump',shattered()),('SM_ResinLimb',burning_limb())]+[
                          ('SM_ExposedRoots'+str(i),roots(801+i*6)) for i in range(3)]:
        bake(name,mesh,palette,DEST)
    u.log('STORMFALL SCULPTURE SAVED')
