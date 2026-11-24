"""Cookable basalt chamber and surveyor's float, authored with Unreal Geometry Script."""
import math
import random
import sys
from pathlib import Path

import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_stormfall as storm
from carpentry_mesh import Joinery, V
from build_log_cabin import bake

DEST = "/Game/NightOfTheWolves/HighWater"
CARP = storm.CARP
STONE, SILT, END, WOOD, IRON, OAK, CALCITE, ROPE, WATER = range(9)
r = random.Random(24031)
u.EditorAssetLibrary.make_directory(DEST)


def surfaces():
    # Thin calcite ebbs along old flood lines, over a scan rather than a flat painted stripe.
    path = DEST + '/M_MineralCrust'
    mat = u.EditorAssetLibrary.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
    if not mat:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset('M_MineralCrust', DEST, u.Material, u.MaterialFactoryNew())
        edit = u.MaterialEditingLibrary
        sample = edit.create_material_expression(mat, u.MaterialExpressionTextureSample, -540, 0)
        sample.set_editor_property('texture', u.load_asset(storm.CASC + 'dry_riverbed_rock_Diffuse'))
        color = edit.create_material_expression(mat, u.MaterialExpressionLinearInterpolate, -230, 0)
        color.set_editor_properties(dict(const_a=0.25, const_b=0.68))
        edit.connect_material_expressions(sample, 'R', color, 'Alpha')
        edit.connect_material_property(color, '', u.MaterialProperty.MP_BASE_COLOR)
        u.EditorAssetLibrary.save_loaded_asset(mat, False)
    basalt = DEST + '/M_WetBasalt'
    if not u.EditorAssetLibrary.does_asset_exist(basalt):
        stone=u.AssetToolsHelpers.get_asset_tools().create_asset('M_WetBasalt', DEST, u.Material, u.MaterialFactoryNew())
        edit=u.MaterialEditingLibrary
        scan=edit.create_material_expression(stone,u.MaterialExpressionTextureSample,-740,-150)
        scan.set_editor_property('texture',u.load_asset(storm.CASC+'dry_riverbed_rock_Diffuse'))
        detail=edit.create_material_expression(stone,u.MaterialExpressionTextureSample,-600,460)
        detail.set_editor_properties(dict(texture=u.load_asset(storm.CASC+'dry_riverbed_rock_nor_dx'),sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL))
        edit.connect_material_property(detail,'RGB',u.MaterialProperty.MP_NORMAL)
        world=edit.create_material_expression(stone,u.MaterialExpressionWorldPosition,-740,90)
        # At this site the full basin is at world Z 401 cm. Uneven pale deposits sit
        # above the darker wetted rock and are interrupted by the scan's fissures.
        common="""
           float seam=403. + 1.8*sin(P.x*.021 + P.y*.014) + 1.1*cos(P.y*.052) + 1.4*sin(P.x*.061);
           float wet=1.-smoothstep(seam-9.,seam+3.,P.z);
           float pale=(1.-smoothstep(2.,9.,abs(P.z-seam-3.)))*(.65+.35*sin(P.y*.039+P.x*.044));
           float shade=dot(S,float3(.25,.53,.22));
        """
        for pin,code,kind,y in [
            (u.MaterialProperty.MP_BASE_COLOR,common+"""
                float3 rock=lerp(float3(.057,.065,.069),float3(.36,.37,.36),saturate(shade*2.1));
                rock*=lerp(1.,.67,wet)*(.93+.07*cos(P.y*.006+P.x*.008));
                return lerp(rock,float3(.52,.48,.36),pale*saturate(shade*3.));
            """,u.CustomMaterialOutputType.CMOT_FLOAT3,0),
            (u.MaterialProperty.MP_ROUGHNESS,common+"return lerp(.82,.36,wet)+.06*shade;",u.CustomMaterialOutputType.CMOT_FLOAT1,250)]:
            term=edit.create_material_expression(stone,u.MaterialExpressionCustom,-270,y)
            pins=[]
            for key in ('S','P'):
                inp=u.CustomInput();inp.set_editor_property('input_name',key);pins.append(inp)
            term.set_editor_properties(dict(code=code,inputs=pins,output_type=kind))
            edit.connect_material_expressions(scan,'RGB',term,'S')
            edit.connect_material_expressions(world,'',term,'P')
            edit.connect_material_property(term,'',pin)
        u.EditorAssetLibrary.save_loaded_asset(stone,False)
    return [u.load_asset(p) for p in [basalt, storm.CASC+'M_DampTread',
            CARP+'M_CutEnds', CARP+'M_AdzedBoards', CARP+'M_ForgedStraps',
            CARP+'M_HewnLogs', path, CARP+'M_TwistedHemp', '/Game/NightOfTheWolves/Crossing/M_RunningWater']]


def face(s, slot, points, toward):
    storm.face(s, slot, [V(*p) for p in points], V(*toward), 160)


def ledge(s, x0, x1, y0, y1, z0, z1=None, base=None, slot=STONE):
    """Top surface follows travel along +Y. Fractured sides cover the collision edges."""
    z1 = z0 if z1 is None else z1
    base = z0-35 if base is None else base
    face(s, slot, [(x0,y0,z0),(x1,y0,z0),(x1,y1,z1),(x0,y1,z1)], (0,0,1))
    for x, n in [(x0,-1),(x1,1)]:
        face(s, STONE, [(x,y0,z0),(x,y1,z1),(x,y1,base),(x,y0,base)], (n,0,0))
    for y, h, n in [(y0,z0,-1),(y1,z1,1)]:
        if abs(h-base) > .05:
            face(s, STONE, [(x0,y,h),(x1,y,h),(x1,y,base),(x0,y,base)], (0,n,0))


def passage(s, sections, floor=True, upper_gallery=False):
    """Vary the spring line and width to keep every vault asymmetrical."""
    rings = []
    outline = [(-1,0),(-.98,.28),(-.91,.61),(-.67,.86),(-.34,.98),
               (.07,1),(.51,.92),(.83,.73),(.96,.38),(1,0)]
    for i,(y,x,width,ground,height) in enumerate(sections):
        ring=[]
        for j,(side,z) in enumerate(outline):
            px=x+width*side+r.uniform(-7,7)
            pz=ground+height*z+r.uniform(-7,7)
            if upper_gallery:
                # The dry shelf reaches the west wall even where the roof narrows,
                # and Max can wade to the east wall at low water. Keep the haunches
                # outside both walking limits up to head height.
                if j<=2:
                    px=min(px,123.+j*9.+7.*math.sin(y*.007))
                    if j==2:
                        pz=max(pz,546.)
                if j>=7:
                    px=max(px,1692.+(j-7)*12.+7.*math.sin(y*.005))
                # Carry the old outer east wall around the new upper tunnel.
                # Its sloping haunch previously crossed the route at head height.
                ease=max(0.,min(1.,(y-2470.)/250.))
                if 7<=j<=8:
                    px+=max(0.,1738.-px)*ease
                if 5<=j<=7:
                    pz+=max(0.,843.-pz)*ease
            ring.append((px,y,pz))
        rings.append(ring)
    for i,(a,b) in enumerate(zip(rings,rings[1:])):
        y,x,width,ground,height=sections[i]
        for j in range(len(a)-1):
            pts=[a[j],a[j+1],b[j+1],b[j]]
            face(s,STONE,pts,(x-(a[j][0]+a[j+1][0])*.5,0,ground+height*.33-(a[j][2]+a[j+1][2])*.5))
        if floor:
            face(s,SILT,[a[0],a[-1],b[-1],b[0]],(0,0,1))
    return rings


def high_gallery(s):
    # From below only the stone lip is visible. A short elbow behind the arch makes
    # the aperture read as a real black passage when light first reaches the sill.
    centers=[(1230,2716,133,264),(1330,2731,142,255),(1420,2771,136,280),
             (1477,2848,141,263),(1503,2965,151,264),(1520,3110,146,261),(1528,3248,141,280)]
    profile=[(-1.,-.08),(-.97,.30),(-.77,.69),(-.54,.87),(-.19,1.),
             (.26,.97),(.66,.86),(.85,.61),(.98,.29),(1.,-.08)]
    rings=[]
    for i,(x,y,w,h) in enumerate(centers):
        prev=centers[max(0,i-1)]; nex=centers[min(len(centers)-1,i+1)]
        d=V(nex[0]-prev[0],nex[1]-prev[1],0).normal()
        cross=V(-d.y,d.x,0)
        ring=[]
        for side,up in profile:
            p=V(x,y,532)+cross*(w*side+r.uniform(-3,3))+V(0,0,h*up+r.uniform(-4,4))
            ring.append(p)
        rings.append(ring)
        if i==0:
            rim=[]
            for j,(side,up) in enumerate(profile):
                rim.append(ring[j]+cross*(36*side)+V(0,0,25*max(0,up))+d*r.uniform(2,9))
            for j in range(len(ring)-1):
                storm.face(s,STONE,[ring[j],rim[j],rim[j+1],ring[j+1]],d*-1,144)
                xx,yy,zz=rim[j].x,rim[j].y,rim[j].z
                if j in (1,2,4,6,8):
                    storm.crag(s,(xx+22,yy,zz),(49,31,31),STONE,9)
    for i,(a,b) in enumerate(zip(rings,rings[1:])):
        x,y,_,_=centers[i]
        for j in range(len(a)-1):
            pts=[a[j],a[j+1],b[j+1],b[j]]
            storm.face(s,STONE,pts,V(x,y,623)-(a[j]+a[j+1])*.5,169)


def back_arch_seams(s, rings):
    # Give each shared section a lip of rock behind it. A ray exactly along
    # the section plane can graze the common edge of *both* adjacent faces.
    for prev,ring,nex in zip(rings,rings[1:],rings[2:]):
        center=V(*[sum(p[axis] for p in ring)/len(ring) for axis in range(3)])
        strips=[[],[]]
        for i,point in enumerate(ring):
            p=V(*point)
            behind=(p-center).normal()*6.
            for strip,other,width in zip(strips,(prev,nex),(13.,19.)):
                step=(V(*other[i])-p)*(width/abs(other[i][1]-point[1]))
                strip.append(p+step+behind)
        storm.stitch(s,*strips)


def shell(cleft_outlet):
    s=Joinery()
    cleft=passage(s,[(908,543,137,20,239),(1010,545,160,47,244),(1165,530,161,96,291),
               (1340,526,172,149,306),(1500,543,208,199,338),(1718,544,230,264,410)])
    # The vault spreads over both the near shelf and the sunken basin.
    stations=[(1680,560,288,247,453),(1810,780,579,11,689),(2000,880,729,8,749),
              (2250,912,786,9,790),(2490,928,802,6,879),(2710,918,799,8,895),
              (2940,922,785,4,860),(3160,987,696,8,830),(3370,1080,632,12,725)]
    basin=passage(s,stations,False,upper_gallery=True)
    # Walls at the two ends meet the bore and upper exit, leaving physical openings.
    for x1,x2 in [(135,320),(760,1750)]:
        face(s,STONE,[(x1,1714,10),(x2,1714,10),(x2,1714,706),(x1,1714,687)],(0,1,0))
    face(s,STONE,[(320,1717,594),(760,1717,605),(760,1717,720),(320,1717,710)],(0,1,0))
    for x1,x2 in [(160,1385),(1685,1730)]:
        face(s,STONE,[(x1,3336,16),(x2,3336,16),(x2,3336,849),(x1,3336,854)],(0,-1,0))
    face(s,STONE,[(1385,3336,8),(1685,3336,8),(1685,3336,532),(1385,3336,532)],(0,-1,0))
    upper=passage(s,[(3235,1529,151,539,285),(3380,1538,145,542,270),(3530,1526,130,551,237),(3670,1490,107,554,200)])
    face(s,STONE,[(1354,3664,540),(1624,3664,540),(1624,3664,780),(1354,3664,780)],(0,-1,0))
    ledge(s,154,1600,1745,3347,26,slot=SILT)
    ledge(s,190,744,1675,1853,264,base=17,slot=SILT)
    ledge(s,172,564,1852,3164,264,base=20,slot=SILT)
    ledge(s,558,861,2605,2814,264,base=42)
    ledge(s,1174,1646,2590,2895,532,base=25)
    ledge(s,1329,1682,2888,3350,532,543,base=24)
    high_gallery(s)
    # Shallow scallops, tilted lips and talus break the collision shelf's straight edge.
    for x,lo,hi,top,span in ((556,2130,2590,264,25),(554,2825,3150,264,25),
                             (1179,2600,2892,532,35),(1329,2895,3330,535,38)):
        for i in range(int((hi-lo)/46)+1):
            y=lo+i*46+r.uniform(-9,9)
            q=r.uniform(.7,1.2)
            storm.crag(s,(x,y,top-31),(span*q,45*q,28*q),STONE,8)
            storm.crag(s,(x,y+r.uniform(-18,18),top-r.uniform(76,144)),
                       (span*r.uniform(.5,1.3),r.uniform(30,72),r.uniform(20,61)),STONE,7)
    for j in range(11):
        x=563+j*27
        storm.crag(s,(x,2809,225),(r.uniform(22,41),r.uniform(17,37),r.uniform(24,41)),STONE,7)
    for i in range(10):
        x=1182+i*45
        storm.crag(s,(x,2596,488),(r.uniform(30,53),r.uniform(19,36),r.uniform(31,47)),STONE,8)
    # Descending steps are broken out of a bedding plane. Each tread overlies the sloping collision.
    for j in range(16):
        x=556+j*41
        h=264-(j+1)*14.84
        ledge(s,x,x+41.5,1892+r.uniform(-12,8),2120+r.uniform(-9,12),h,base=18)
    # A split in the overhang feeds the pivoting trough. The bypass is a stone runnel
    # with a raised rim; diverted water disappears into a dark slot behind its end.
    for i in range(8):
        x=170+i*52
        storm.crag(s,(x,2320,612-i*3),(r.uniform(65,83),r.uniform(58,85),r.uniform(37,62)),STONE,9)
    for i in range(5):
        storm.crag(s,(526+i*2,2327,304+i*2),(35-i*3,39-i*3,36-i*4),STONE,8)
    for i in range(8):
        t=i/7
        x=649+27*t; y=2422+120*t; z=294-37*t
        storm.crag(s,(x,y,z-23),(44,25,27),STONE,8)
        for side in (-1,1):
            storm.crag(s,(x+side*22,y,z-8),(10,23,15),STONE,7)
    storm.crag(s,(680,2561,184),(86,51,87),STONE,9)
    # Exposed ladder end knocks a rock nose just above the initial waterline.
    storm.crag(s,(1212,2257,265),(73,51,48),STONE,9)
    storm.crag(s,(1232,2228,129),(75,75,143),STONE,8)
    # Scalloped crust of calcite on the eroded drop below Max's boots and on the far walls.
    for x,y,dx,dy,normal in [(565,2135,0,1,(1,0,0)),(1180,2588,1,0,(0,-1,0)),(720,3330,1,0,(0,-1,0))]:
        for i in range(30):
            t=i*18.; length=r.uniform(14.,22.)
            h=249+1.5*math.sin(t*.026)+r.uniform(-1.,1.)
            face(s,CALCITE,[(x+t*dx,y+t*dy,h),(x+(t+length)*dx,y+(t+length)*dy,h+.8),
                          (x+(t+length)*dx,y+(t+length)*dy,h+r.uniform(3,7)),(x+t*dx,y+t*dy,h+r.uniform(4,9))],normal)
    # Mineral ribs and beds vary in scale, preserving clear foot paths and the camera line.
    for i in range(58):
        y=r.uniform(1820,3300); side=r.choice((-1,1))
        x=(146 if side<0 else 1682)+r.uniform(-20,18)
        z=r.choice((83,140,476,657))
        extent=(r.uniform(42,91),r.uniform(93,181),r.uniform(46,109))
        # Recess the roots of the thick ribs into the bedrock so their tips
        # stop at the standing edge, especially along the raised east gallery.
        x=min(x,161.-1.3*extent[0]) if side<0 else max(x,1666.+1.3*extent[0])
        storm.crag(s,(x,y,z),extent,STONE,9)
    for i in range(33):
        y=r.uniform(1880,3240); x=r.choice((146,1518,1570))+r.uniform(-36,30)
        z=26 if x>1300 else 263
        q=r.uniform(6,21); storm.crag(s,(x,y,z),(q*1.4,q,q*.70),STONE,7)
    # Pewter-colored fissure under the drain, surrounded by tilted edges and shingle.
    for i in range(11):
        y=2210+i*11.
        storm.crag(s,(1450+r.uniform(-22,23),y,26),(r.uniform(18,28),16,r.uniform(10,24)),STONE,7)
    # Surveyor's guideposts, both rubbed smooth in line with the platform.
    for y in (2585,2835):
        for x in (930,1094):
            s.timber((x,y,20),(x+1.2,y,308),6.7,OAK,END,12)
            for dz in (52,75,244,257):
                s.timber((x,y,dz),(x,y,dz+2),7.0,IRON,IRON,12)
    # Overlapping arches still have gaps when viewed obliquely through their rims.
    # Join their actual vertices, including the floor beneath the shallow basin.
    decor_state=storm.rng.getstate()
    storm.stitch(s,cleft_outlet,[V(*p) for p in cleft[1]])
    for start,finish in ((cleft[-1],basin[0]),(basin[-1],upper[1])):
        storm.stitch(s,[V(*p) for p in start],[V(*p) for p in finish])
    for a,b in zip(basin,basin[1:]):
        face(s,SILT,[a[0],a[-1],b[-1],b[0]],(0,0,1))
    face(s,STONE,upper[-1],(0,-1,0))
    for rings in (cleft,basin,upper):
        back_arch_seams(s,rings)
    storm.rng.setstate(decor_state)
    return s


def fork():
    s=Joinery()
    s.plank((-1,0,-31),(-1,0,36),16,4,(0,1,0),IRON)
    for y in (-1,1):
        s.timber((2,y*5,-12),(15,y*15,1),2.6,IRON,IRON,8)
        s.timber((15,y*15,1),(19,y*10,22),2.5,IRON,IRON,8)
    for z in (-23,27):
        s.timber((-1,0,z),(3,0,z),3.5,IRON,IRON,9)
    return s


def work_float():
    s=Joinery()
    for y in (-65,0,66):
        s.timber((-135,y,-17),(137,y,-17),13.4,OAK,END,15,r2=12.)
    for i in range(14):
        x=-127+i*19.4
        s.plank((x,-95+r.uniform(-2,2),-4),(x,95+r.uniform(-2,2),-4),18.6,8,(1,0,0),WOOD,1.4)
    # Ladder pitches inward toward the lip. Leave the deck center open to board from shore.
    for y in (-26,26):
        s.plank((82,y,-2),(157,y,284),8.3,7,(0,1,0),OAK,1.6)
        s.plank((29,y,0),(107,y,91),6.5,4.8,(0,1,0),WOOD)
        for z in (32,87):
            s.timber((82+.264*z,y-5,z),(82+.264*z,y+5,z),2.5,IRON,IRON,8)
    for i in range(9):
        z=20.5+i*29
        s.timber((82+.264*z,-28,z),(82+.264*z,28,z),3.3,OAK,END,10)
    for y in (-1,1):
        for x in (-107,105):
            s.timber((x,y*92,-12),(x,y*112,-13),3.2,IRON,IRON,9)
    # The surveyor could shove from the silt with both hands on the braced stern bar.
    for x in (-45,45):
        s.timber((x,-77,-2),(x,-77,76),4.3,OAK,END,12)
        s.plank((x,-55,0),(x,-77,48),5,3,(1,0,0),WOOD)
        s.timber((x,-82,10),(x,-72,10),2.1,IRON,IRON,9)
    s.timber((-51,-77,76),(51,-77,76),3.9,OAK,END,14)
    return s


def trough():
    s=Joinery()
    # Green split cedar with a deep open channel, iron shoe, hand grip and a pivot.
    for y,ang in ((-13,-1),(13,1)):
        s.plank((-31,y*0.63,1),(151,y,-14),17,4.8,(0,ang*.59,.81),WOOD,1.5)
    s.plank((-34,0,-5),(154,0,-23),13,5,(0,1,0),WOOD)
    s.timber((0,0,-9),(0,0,-28),8,IRON,IRON,12)
    s.timber((24,-8,-4),(4,-54,14),4.0,OAK,END,12)
    return s


def timber():
    s=Joinery()
    s.timber((-57,-35,0),(66,42,3),16,OAK,END,13)
    s.timber((23,12,6),(37,7,30),6.1,OAK,END,9,r2=4)
    return s


def brand():
    s=Joinery()
    s.timber((0,0,-42),(0,0,34),3.4,OAK,END,12,r2=4)
    for i in range(10):
        z=16+i*2.4
        s.timber((0,0,z),(0,0,z+2.7),5.3+.9*math.sin(i*.35),ROPE,ROPE,12)
    return s


def water_ribbon():
    s=Joinery()
    for j in range(6):
        a=j/6; b=(j+1)/6
        for i in range(4):
            y=-13+i*6.
            edge=.75 if i in (0,3) else 1.
            def pt(t,w):
                return (23*math.sin(t*math.pi/2),w*edge+1.7*math.sin(7*t+i),-100*t)
            face(s,WATER,[pt(a,y),pt(a,y+6),pt(b,y+6),pt(b,y)],(1,0,0))
            face(s,WATER,[pt(a,y),pt(b,y),pt(b,y+6),pt(a,y+6)],(-1,0,0))
    return s


def bypass():
    s=Joinery()
    for i in range(8):
        a=i/8; b=(i+1)/8
        def pt(t,side):
            return (649+27*t+side*(8.6+1.7*math.sin(t*9)),2422+t*120,294.9-37*t)
        face(s,WATER,[pt(a,-1),pt(a,1),pt(b,1),pt(b,-1)],(0,0,1))
    return s


def camera_shell(mesh):
    body=mesh.get_editor_property('body_setup')
    body.set_editor_properties(dict(collision_trace_flag=u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
                                    double_sided_geometry=True))
    u.EditorAssetLibrary.save_loaded_asset(mesh,False)


def line_old_cleft(bluff):
    # The earlier prop atlas has unused black UV islands on the wide basalt planes.
    # Both sides of the tunnel join need the same tileable fractured-rock scan.
    bluff.set_material(0,u.load_asset(DEST+'/M_WetBasalt'))
    u.EditorAssetLibrary.save_loaded_asset(bluff,False)


if __name__ == '__main__':
    palette=surfaces()
    if not all(palette):
        raise RuntimeError('Missing chamber surfaces')
    cleft_outlet=[]
    bluff=bake('SM_RivenBluff',storm.cave(cleft_outlet),storm.palette,storm.DEST)
    camera_shell(bluff)
    line_old_cleft(bluff)
    for name,sculpt in [('SurveyCavern',lambda:shell(cleft_outlet)),('SurveyFloat',work_float),('SpringTrough',trough),
                        ('WedgedTimber',timber),('IronTorchFork',fork),('SurveyBrand',brand),('FallingWater',water_ribbon),
                        ('BypassFlow',bypass)]:
        mesh=bake('SM_'+name,sculpt(),palette,DEST)
        if name=='SurveyCavern':
            camera_shell(mesh)
    u.log('HIGH WATER SCENERY BAKED')
