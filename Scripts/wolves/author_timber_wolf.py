"""Build the skinned wolf, guard-hair atlas and death-card-derived face markings.

Run with Python 3 and ImageMagick 7. Authoring space is centimetres, facing +X.
"""
import json
import math
import random
import subprocess
import zlib
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parents[2]
SOURCE = HERE / "Art/Source/FirstLeg/Wolves/SK_TimberWolf.glb"
bones, joints = [], {}
RNG = random.Random(9073)
SLOTS = ("TimberCoat", "TimberRuff", "TimberFace", "TimberWet", "TimberEye")
faces = {slot: [] for slot in SLOTS}
projection = []
points, normals, colors, uvs, ids, weights = [], [], [], [], [], []
sine, cosine, tau = math.sin, math.cos, math.tau

def xyz(p):
    return (p[0] / 100, p[2] / 100, p[1] / 100)

def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))

def bone(name, pos, parent=None):
    joints[name] = len(bones)
    bones.append((name, pos, parent))

bone("Root", (0, 0, 0))
bone("Ribcage", (0, 0, 52), "Root")
bone("Neck", (23, 0, 62), "Ribcage")
bone("Skull", (44, 0, 76), "Neck")
bone("Jaw", (49, 0, 73), "Skull")
bone("Brush", (-46, 0, 58), "Ribcage")
bone("BrushTip", (-67, 0, 35), "Brush")
for left, side in (("L", -1), ("R", 1)):
    bone("Ear" + left, (42, side * 9, 85), "Skull")
    for fore in (True, False):
        limb = ("Fore" if fore else "Hind") + left
        x = 25 if fore else -37
        knee = 20 if fore else -22
        ankle = 23 if fore else -42
        chain = ["Ribcage"]
        for tag, p in (("Upper", (x, side * 12, 56)), ("Lower", (knee, side * 13, 31)),
                       ("Pastern", (ankle, side * 13.5, 12)), ("Paw", (ankle + 2, side * 13.5, 5))):
            name = limb + tag
            bone(name, p, chain[-1])
            chain.append(name)

def binding(w):
    if isinstance(w, str):
        return [(joints[w], 1.0)]
    return [(joints[k], v) for k, v in w]

def add(a,b):
    return tuple(x+y for x,y in zip(a,b))

def mul(a,k):
    return tuple(x*k for x in a)

def dot(a,b):
    return sum(x*y for x,y in zip(a,b))

def cross(a,b):
    return a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]

def unit(a):
    return mul(a,1./max(.00001,math.sqrt(dot(a,a))))

def clamp(t):
    return max(0.,min(1.,t))

def mix(a,b,t):
    t=clamp(t)
    return add(mul(a,1-t),mul(b,t))

def two(a,b,t):
    return ((a,1-t),(b,t))

def projected(p):
    # Front-on pigmentation plate from the project's 569 x 520 composition.
    # The eyes, bridge and upper muzzle lie on successive depth planes.
    x,y,z = p
    return ((315.5 + 3.81*y - 159.)/306.,
            (166.7 - 5.20*(z-81.7) + 1.28*(x-55.4) - 3.)/318.)

def vertex(p,n,tint,joint,uv=(0,0),alpha=1,faceuv=None):
    points.append(xyz(p))
    n=unit(n)
    normals.append((n[0],n[2],n[1]))
    colors.append((*[clamp(x) for x in tint],clamp(alpha)))
    uvs.append(uv)
    projection.append(projected(p) if faceuv is None else faceuv)
    slots=binding(joint)
    ids.append(tuple([b for b,v in slots]+[0]*(4-len(slots))))
    weights.append(tuple([v for b,v in slots]+[0.]*(4-len(slots))))
    return len(points)-1

def face(slot,a,b,c):
    delta=cross(sub(points[b],points[a]),sub(points[c],points[a]))
    if dot(delta,add(normals[a],add(normals[b],normals[c])))<0:
        b,c=c,b
    faces[slot].extend((a,b,c))

def quad(slot,a,b,c,d):
    face(slot,a,b,c)
    face(slot,a,c,d)

def coat(p,v):
    x,y,z=p
    swath=.065*sine(x*.15+y*.12)+.05*sine(.31*x-.16*z)
    value=clamp((z-35)/41+swath)
    # Dark dorsal saddle, silver flanks and pale pasterns.
    return mix(mix((.45,.45,.427),(.114,.126,.142),value),(.52,.53,.517),clamp((-v-.24)*.75))

def mantle(p,v):
    x,y,z=p
    return mix((.43,.45,.458),(.17,.19,.212),.52+.38*v+.09*sine(x*.32+y*.21))

def cylinder_point(a,b,t,theta,axis):
    x,y,z,width,height=[a[i]+(b[i]-a[i])*t for i in range(5)]
    u,v=cosine(theta),sine(theta)
    if axis=="x":
        pos=(x,y+width*u,z+height*v)
        along=b[0]-a[0]
        slide=((b[1]-a[1]+u*(b[3]-a[3]))*u*height+(b[2]-a[2]+v*(b[4]-a[4]))*v*width)
        n=(-slide/(along or .001),u*height,v*width)
    else:
        pos=(x+width*u,y+height*v,z)
        along=b[2]-a[2]
        slide=((b[0]-a[0]+u*(b[3]-a[3]))*u*height+(b[1]-a[1]+v*(b[4]-a[4]))*v*width)
        n=(u*height,v*width,-slide/(along or .001))
    return pos,unit(n)

def loft(stations,color,axis="x",sides=24,slot="TimberCoat",hair=None):
    rows=[]
    for i,a in enumerate(stations):
        row=[]
        for j in range(sides+1):
            theta=tau*j/sides
            p,n=cylinder_point(a,stations[min(i+1,len(stations)-1)],0.,theta,axis)
            if i:
                _,prev=cylinder_point(stations[i-1],a,1.,theta,axis)
                n=prev if i==len(stations)-1 else unit(add(prev,n))
            tint=color(p,sine(theta)) if callable(color) else color
            # Fur runs along the trunk and down the legs. A duplicated seam keeps
            # the lengthwise pattern fixed to the skin as each joint rotates.
            v=(-p[0]-.32*p[2])/25. if axis=="x" else -p[2]/16.
            uv=(j/sides*(4 if axis=="x" else 1),v)
            alpha=clamp((p[0]-40)/12.) if slot=="TimberFace" else 1.
            row.append(vertex(p,n,tint,a[5],uv,alpha))
        rows.append(row)
    for a,b in zip(rows,rows[1:]):
        for j in range(sides):
            quad(slot,a[j],a[j+1],b[j+1],b[j])
    for row in (rows[0],rows[-1]):
        for j in range(1,sides-1):
            face(slot,row[0],row[j],row[j+1])
    if hair:
        layer_fur(stations,axis,color,*hair)

def ellipsoid(pos,radii,joint,color,slot="TimberCoat",long=24,lat=12,forward=(1,0,0)):
    f=unit(forward)
    r=unit(cross((0,0,1),f))
    up=unit(cross(f,r))
    sides=[f,r,up]
    rows=[]
    for i in range(lat+1):
        phi=-math.pi/2+math.pi*i/lat
        row=[]
        for j in range(long+1):
            t=tau*j/long
            coeff=(cosine(phi)*cosine(t),cosine(phi)*sine(t),sine(phi))
            offset=(0,0,0); n=(0,0,0)
            for k in range(3):
                offset=add(offset,mul(sides[k],coeff[k]*radii[k]))
                n=add(n,mul(sides[k],coeff[k]/radii[k]))
            xyzpos=add(pos,offset)
            c=color(xyzpos,coeff[2]) if callable(color) else color
            row.append(vertex(xyzpos,n,c,joint,(j/long*2,(1-i/lat)),
                              clamp((xyzpos[0]-39)/12) if slot=="TimberFace" else 1.))
        rows.append(row)
    for a,b in zip(rows,rows[1:]):
        for j in range(long):
            quad(slot,a[j],a[j+1],b[j+1],b[j])

def interpolated_weights(a,b,t):
    merged={}
    for group,s in ((a,1-t),(b,t)):
        for idx,weight in binding(group):
            name=bones[idx][0]
            merged[name]=merged.get(name,0.)+weight*s
    return [(name,weight) for name,weight in merged.items() if weight>.000001]

def tuft(p,n,flow,length,width,color,joint):
    # These bent ribbons carry actual alpha-masked clusters of fine tapered guard
    # hairs. Their normals follow the coat, making overlapping rows shade as one pelt.
    along=unit(sub(flow,mul(n,dot(flow,n))))
    if dot(along,along)<.1: return
    across=unit(cross(along,n))
    tile=RNG.randrange(8)
    bend=RNG.uniform(-.14,.14)*length
    root=add(p,mul(n,-min(.52,length*.045)))
    rise=RNG.uniform(.085,.26)
    rows=[]
    for v,span in ((0,.85),(.34,1.),(.7,.70),(1,.20)):
        center=add(root,add(mul(along,length*v),add(mul(n,length*rise*v*(1.55-.55*v)),mul(across,bend*v*v))))
        row=[]
        for j in (-1,0,1):
            pos=add(center,add(mul(across,j*width*span/2),mul(n,(1-j*j)*width*.09)))
            tint=mul(color,(.88+.22*v))
            row.append(vertex(pos,unit(add(n,mul(across,j*.12))),tint,joint,
                              (((tile%4)+(j+1)*.5*.96+.02)/4,((tile//4)+v*.98+.01)/2)))
        rows.append(row)
    for a,b in zip(rows,rows[1:]):
        for i in (0,1):
            quad("TimberRuff",a[i],a[i+1],b[i+1],b[i])

def layer_fur(stations,axis,color,density,length,width,kind):
    for a,b in zip(stations,stations[1:]):
        distance=math.sqrt(sum((a[k]-b[k])**2 for k in range(3)))
        circumference=math.pi*.5*(a[3]+a[4]+b[3]+b[4])
        count=round(distance*circumference*density)
        for _ in range(count):
            t=RNG.random()
            angle=RNG.random()*tau
            p,n=cylinder_point(a,b,t,angle,axis)
            s=-1 if p[1]<0 else 1
            if kind=="ruff":
                flow=(-.94,s*.20,-.69+.63*max(0,n[2]))
            elif kind=="cheek":
                flow=(-.88,s*.27,-.67+.64*max(0,n[2]))
                if p[0]>50 and abs(p[1])<13 and 79<p[2]<86: continue
            elif kind=="tail":
                flow=(-.8,s*.08,-.57)
            elif kind=="limb":
                flow=(-.35,s*.03,-1.)
            else:
                flow=(-1.,s*.06,-.65+.65*max(0,n[2]))
            flow=add(flow,(RNG.uniform(-.15,.15),RNG.uniform(-.22,.22),RNG.uniform(-.18,.18)))
            c=color(p,sine(angle)) if callable(color) else color
            salt=RNG.uniform(.76,1.14)
            if RNG.random()<.075: salt+=.17
            tuft(p,n,flow,length*RNG.triangular(.38,1.4,.82),width*RNG.uniform(.54,1.22),mul(c,salt),
                 interpolated_weights(a[5],b[5],t))

pale=(.53,.54,.528)
ivory=(.75,.67,.52)
wet="TimberWet"
face_slot="TimberFace"
# Ribcage, with a thick mantle at the withers and tucked abdomen.
body=[(-53,0,54,2.8,7,"Ribcage"),(-47,0,54,12.3,17,"Ribcage"),(-38,0,53,16.6,20.7,"Ribcage"),
      (-26,0,55,14.1,17.3,"Ribcage"),(-16,0,55.5,13.5,16.5,"Ribcage"),(-6,0,55,16.7,19.8,"Ribcage"),
      (7,0,55.6,18.2,24,"Ribcage"),(18,0,58,19,24.3,"Ribcage"),
      (26,0,62,18.8,22,two("Ribcage","Neck",.31)),(33,0,66,17.5,19,two("Ribcage","Neck",.65)),(41,0,67,10.8,13.5,"Neck")]
loft(body,coat,sides=46,hair=(.19,8.9,2.15,"body"))
# The flared cheeks and deep throat taper sharply into the bridge and black nose.
neck=[(20,0,65,14,14,two("Ribcage","Neck",.45)),(26,0,67,17.5,19,two("Ribcage","Neck",.72)),
      (32,0,70,20,22,"Neck"),(37,0,73,20,21,two("Neck","Skull",.24)),
      (42,0,75.7,17.8,17,two("Neck","Skull",.66)),(47,0,76.1,15,15.2,"Skull")]
loft(neck,mantle,sides=42,hair=(.38,11.9,1.96,"ruff"))
head=[(38,0,76.2,13.5,13.2,"Skull"),(43,0,77.2,15.9,15.6,"Skull"),(48,0,78.6,15.6,14.6,"Skull"),
      (52,0,79.3,14.4,12.7,"Skull"),(55.3,0,78.7,12.8,11.1,"Skull"),(58.1,0,77.2,10.2,8.9,"Skull"),
      (61.8,0,75,8.1,7.0,"Skull"),(65.6,0,73.7,7.6,5.5,"Skull"),(69,0,73.2,7.1,4.25,"Skull"),
      (72.3,0,73.5,5.1,3.5,"Skull")]
loft(head,mantle,sides=40,slot=face_slot)
layer_fur(head[:5],"x",mantle,.43,5.9,1.31,"cheek")
layer_fur(head[4:7],"x",mantle,.23,2.7,.89,"cheek")
# The side view keeps a distinct lower jaw, tongue and upper and lower gums when the pounce opens the mouth.
loft([(48,0,70,7.8,4.3,"Jaw"),(53.5,0,68.6,8.4,4.9,"Jaw"),(59.7,0,67.3,7.6,4.1,"Jaw"),
      (66,0,67.6,5.7,2.8,"Jaw"),(70.3,0,68.3,4.2,2.1,"Jaw"),(72,0,68.7,2.5,1.3,"Jaw")],(.35,.35,.34),hair=(.34,3.1,1.15,"cheek"))
ellipsoid((60.1,0,69.5),(11.8,7.05,1.6),"Jaw",(.12,.045,.054),wet)
ellipsoid((59.2,0,68.4),(10.7,6.7,1.35),"Skull",(.12,.04,.045),wet)
ellipsoid((60.5,0,69.95),(8.9,4.25,1.18),"Jaw",(.34,.116,.133),wet)
def rhinarium(u,v):
    # Two lateral nares bend back around a low, triangular black pad.
    radius=math.hypot(u,v)
    h=abs(u)
    height=v-.03-.36*(h-.62)
    opening=math.exp(-((h-.59)/.226)**2-(height/.19)**2)
    wing=math.exp(-((h-.65)/.31)**2-((height-.22)/.15)**2)
    cleft=math.exp(-(u/.055)**2)*clamp((-.06-v)*2.8)
    x=73.95+1.63*max(0.,1.-radius*radius)**.74-1.25*opening+.32*wing-.34*cleft
    y=6.05*u*(.90+.21*v)
    z=74.27+3.33*v+.43*h*h
    pigment=mix((.034,.036,.038),(.003,.003,.004),clamp(opening*1.30+cleft*.46))
    return (x,y,z),pigment

def nose_pad():
    rows=[]
    for radius in [i/29 for i in range(30)]:
        row=[]
        for i in range(97):
            a=tau*i/96
            u,v=radius*cosine(a),radius*sine(a)
            p,col=rhinarium(u,v)
            du=sub(rhinarium(u+.004,v)[0],rhinarium(u-.004,v)[0])
            dv=sub(rhinarium(u,v+.004)[0],rhinarium(u,v-.004)[0])
            row.append(vertex(p,unit(cross(du,dv)),col,"Skull",(u*.48+.5,.5-v*.48),0.))
        rows.append(row)
    for a,b in zip(rows,rows[1:]):
        for i in range(96): quad(wet,a[i],a[i+1],b[i+1],b[i])
    for depth,scale in ((72.6,1.01),(70.5,.91),(69.6,.78)):
        row=[]
        for i in range(97):
            a=tau*i/96; u,v=cosine(a),sine(a)
            (_,y,z),col=rhinarium(u,v)
            p=(depth,y*scale,74.27+(z-74.27)*scale)
            row.append(vertex(p,unit((.18,u,v)),col,"Skull",(.5+.47*u,.5-.47*v),0.))
        for i in range(96): quad(wet,rows[-1][i],rows[-1][i+1],row[i+1],row[i])
        rows.append(row)
nose_pad()
for s in (-1,1):
    lip=[(55.8,s*7,70.4,1.2,.58,"Skull"),(58.6,s*8.1,69.9,1.12,.95,"Skull"),
         (62,s*7.65,69.85,.92,1.02,"Skull"),(65.4,s*6.95,69.7,.80,.87,"Skull"),
         (68.5,s*6.0,70.0,.73,.61,"Skull"),(71.2,s*4.65,70.55,.5,.43,"Skull")]
    loft(lip,(.062,.057,.059),sides=16,slot=wet)
    flews=[(57.3,s*6.74,72.68,.46,1.64,"Skull"),(60.7,s*7.02,72.2,1.76,2.28,"Skull"),
           (64.2,s*6.05,72,2.38,2.03,"Skull"),(67.5,s*5.3,71.85,2.27,1.54,"Skull"),(70.7,s*4.2,72.4,1.27,1.21,"Skull")]
    loft(flews,(.55,.54,.50),sides=20,slot=face_slot)
    for i in range(6):
        x=60.8+i*.93
        y=8.65-(x-60.8)*.13
        z=71.7+(.79 if i%2 else 0)
        ellipsoid((x,s*y,z),(.16,.13,.17),"Skull",(.075,.073,.069),wet,long=8,lat=5)
        # Tapered whiskers sweep back around the flews.
        length=4.8+(i%3)*1.6
        axis=unit((-.22,s*.73,.20))
        across=unit(cross(axis,(1.,0.,0.)))
        rows=[]
        for t,width in ((0,.075),(.43,.053),(.76,.034),(1,.008)):
            pos=(x+1.7*t-3.9*t*t,s*(y+length*t),z+1.0*t-1.28*t*t)
            rows.append([vertex(add(pos,mul(across,q*width)),axis,(.45,.45,.42),"Skull") for q in (-1,1)])
        for a,b in zip(rows,rows[1:]): quad(wet,a[0],a[1],b[1],b[0])
    for x,z,bone_,direction,extent,diam in [(66.5,69.6,"Skull",-1,4.0,1.02),(59.8,69.1,"Skull",-1,2.05,.86),
                            (69.0,70,"Jaw",1,2.6,.82),(63,69.6,"Jaw",1,1.15,.57)]:
        y=s*(5.45 if x>63 else 6.35)
        loft([(x,y,z,diam,diam*.81,bone_),(x+.07,y,z+direction*extent*.44,diam*.77,diam*.7,bone_),
              (x-.43,y,z+direction*extent*.91,diam*.27,diam*.25,bone_),(x-.58,y,z+direction*extent,.08,.07,bone_)],ivory,"z",12,wet)
    for i in range(3):
        y=s*(.7+i*1.25); x=70.95-i*.38
        loft([(x,y,70.3,.69,.65,"Skull"),(x+.03,y,69.63,.59,.54,"Skull"),(x-.13,y,69.14,.24,.32,"Skull")],ivory,"z",8,wet)

def eye(side):
    # Both eyes look obliquely forward. Almond sockets have elevated outer
    # corners, a projecting lower lid, and a circular iris behind a narrowed brow.
    f=unit((.76,.65*side,.10))
    outer=unit((-.65,side*.76,0))
    up=unit(cross(f,outer))
    if up[2]<0: up=mul(up,-1)
    center=(55.0,side*12.15,82.9)
    def place(u,v,depth):
        return add(center,add(mul(outer,u),add(mul(up,v+.185*u),mul(f,depth))))
    def disk(rings,slot):
        rows=[]
        for rx,rz,depth,color in rings:
            row=[]
            for j in range(33):
                a=tau*j/32
                h=rx*cosine(a); v=rz*sine(a)*(1-.16*abs(cosine(a)))
                p=place(h,v,depth)
                n=unit(add(f,add(mul(outer,.22*cosine(a)),mul(up,.18*sine(a)))))
                row.append(vertex(p,n,color,"Skull",(j/32,rx/4.5),1))
            rows.append(row)
        for l,r in zip(rows,rows[1:]):
            for j in range(32): quad(slot,l[j],l[j+1],r[j+1],r[j])
    disk([(4.07,2.12,-.31,(.13,.13,.13)),(3.45,1.73,.44,(.062,.055,.053)),(2.73,1.22,.93,(.021,.015,.012)),
          (.03,.03,1.07,(.009,.008,.008))],wet)
    disk([(1.70,1.16,1.10,(.042,.012,.017)),(1.41,1.04,1.25,(.18,.022,.031)),
          (.97,.79,1.42,(.54,.040,.058)),(.62,.65,1.54,(.50,.067,.075)),(.48,.52,1.57,(.010,.009,.011)),(.025,.025,1.61,(.008,.007,.009))],"TimberEye")
    glint=place(-.56,.54,1.7)
    ellipsoid(glint,(.12,.27,.20),"Skull",(.75,.80,.74),"TimberEye",10,6,forward=f)
    # Brow fur overhang is rooted near the rim so the socket remains readable in profile.
    eyebrow=[(50.5,side*13.7,85.3,.7,1.1,"Skull"),(52.7,side*13.22,85.5,2,1.5,"Skull"),
             (55.4,side*11.4,84.8,2.3,1.3,"Skull"),(57.6,side*9.3,83.35,1.45,.77,"Skull"),(58.2,side*8.2,82.9,.4,.35,"Skull")]
    loft(eyebrow,mantle,sides=16,slot=face_slot)

def pinna(side,ear):
    # The recessed concha and its rolled silver rim are one folded surface.
    # Front and rear meet at the same tip and contour at every view angle.
    def point(h,theta):
        u,v=cosine(theta),sine(theta)
        radius=.045+.955*(1.-h)**.72
        pocket=max(0.,u)**3.1*sine(math.pi*h)**.58
        x=42.45-4.38*h+5.05*radius*u-3.38*radius*pocket
        y=side*(10.15+4.8*h)+5.23*radius*v
        return (x,y,85.55+15.97*h)
    rows=[]
    for i in range(25):
        h=i/24
        row=[]
        for j in range(41):
            theta=tau*j/40
            du=sub(point(h,theta+.005),point(h,theta-.005))
            dv=sub(point(min(1.,h+.002),theta),point(max(0.,h-.002),theta))
            n=unit(cross(du,dv))
            # Dense pale fur around the helix graduates into shorter, darker
            # fur within the bowl without a second cap poking through the pinna.
            front=clamp((cosine(theta)-.26)/.47)
            bowl=clamp((h-.09)/.18)*clamp((.91-h)/.2)*front
            rim=clamp((.59-abs(cosine(theta)-.37)))*.62
            base=mix((.17,.185,.20),(.49,.50,.49),rim)
            tint=mix(base,(.067,.060,.061),bowl)
            row.append(vertex(point(h,theta),n,tint,two("Skull",ear,clamp(h/.19)),
                              (j/40*1.8,(1.-h)*.81)))
        rows.append(row)
    for a,b in zip(rows,rows[1:]):
        for j in range(40): quad("TimberCoat",a[j],a[j+1],b[j+1],b[j])
    for j in range(1,39): face("TimberCoat",rows[-1][0],rows[-1][j],rows[-1][j+1])
    for _ in range(236):
        h=RNG.uniform(.06,.91)
        theta=RNG.choice((-1.,1.))*RNG.uniform(.77,1.66)
        p=point(h,theta)
        du=sub(point(h,theta+.005),point(h,theta-.005))
        dv=sub(point(h+.002,theta),point(h-.002,theta))
        n=unit(cross(du,dv))
        tuft(p,n,(.26,-.33*sine(theta),-.94),RNG.uniform(.86,2.17),RNG.uniform(.25,.58),
             mul(pale,RNG.uniform(.71,.99)),ear)

for end,s in (("L",-1),("R",1)):
    eye(s)
    pinna(s,"Ear"+end)
    for fore in (True,False):
        limb=("Fore" if fore else "Hind")+end
        upper,lower,ankle,paw=[limb+n for n in ("Upper","Lower","Pastern","Paw")]
        x=25 if fore else -37; k=20 if fore else -22; h=23 if fore else -42
        leg=[(x-1.,s*11.55,61,11.4,7.65,two("Ribcage",upper,.43)),(x,s*12,53,10.7,7.65,upper),
             ((x+k)/2,s*13,42,8.35,6.5,upper),(k,s*13,31,5.65,4.8,two(upper,lower,.6)),
             ((k+h)/2,s*13.55,22,3.9,3.65,lower),(h,s*13.5,12,3.4,3.15,two(lower,ankle,.58)),(h+2,s*13.5,5.4,3.2,3.1,ankle)]
        loft(leg,coat,"z",24,hair=(.12,4.7,1.52,"limb"))
        ellipsoid((h+3.1,s*13.5,3.8),(6.25,4.9,3.45),paw,pale)
        for i,off in enumerate((-3.42,-1.1,1.1,3.42)):
            x=h+7.4-abs(off)*.42
            ellipsoid((x,s*13.5+off,2.78),(3.7,1.64,2.1),paw,(.366,.365,.348),long=12,lat=7)
            loft([(x+2.15,s*13.5+off,2.2,.64,.7,paw),(x+3.82,s*13.5+off,1.82,.50,.63,paw),
                  (x+4.8,s*13.5+off,1.22,.13,.19,paw)],(.10,.086,.069),sides=8,slot=wet)
tail=[(-93,4,18,.5,.9,"BrushTip"),(-86,3.5,21.3,5.3,6.4,"BrushTip"),(-79,3,25.5,8.8,9.9,"BrushTip"),
      (-69,2,32,9.1,11.8,"BrushTip"),(-61,1.5,41,8.6,10.8,two("Brush","BrushTip",.31)),(-53,.5,51,7.3,8.4,"Brush"),(-46,0,57,4.8,6.1,"Brush")]
loft(tail,coat,sides=26,hair=(.25,10.7,2.11,"tail"))

def write_fur_atlas(path):
    # Eight separate combed locks. Hair widths and lengths vary continuously so
    # the masked cards produce ragged tips instead of repeating solid triangles.
    rng=random.Random(46107)
    tw,th=256,512
    width,height=tw*4,th*2
    rgba=bytearray(width*height*4)
    for tile in range(8):
        ox,oy=tile%4*tw,tile//4*th
        lean=rng.uniform(-.15,.15)
        # Clusters of narrow filaments leave negative space between locks even
        # halfway along a card. Their ragged, separated tips blend with shorter cards.
        bundles=[(rng.uniform(.09,.91),rng.uniform(.56,.97)) for _ in range(13)]
        for strand in range(76):
            bundle,limit=rng.choice(bundles)
            root=bundle+rng.gauss(0,.035)
            reach=clamp(limit+rng.gauss(0,.082))
            kink=rng.uniform(-.07,.07)
            side=(root-.5)
            comb=rng.uniform(-.085,.085)
            shade=rng.uniform(.35,.98)
            radius=rng.uniform(.45,1.05)
            for iy in range(int(th*reach)):
                t=(iy+.5)/(th*reach)
                spread=(1.+.12*sine(t*math.pi))*(1.-.42*t*t)
                center=.5+side*spread+lean*t+comb*t*t+kink*sine(math.pi*t)
                fx=center*tw
                r=radius*(1.-.85*t*t)
                tone=int(255*clamp(shade*(.59+.34*t+.12*sine(math.pi*t))))
                for ix in range(max(0,int(fx-r-1)),min(tw,int(fx+r+2))):
                    coverage=clamp(r+.57-abs(ix+.5-fx))
                    if coverage<=0: continue
                    i=((iy+oy)*width+ix+ox)*4
                    old=rgba[i+3]/255.
                    alpha=coverage+old*(1-coverage)
                    col=round((tone*coverage+rgba[i]*old*(1.-coverage))/alpha)
                    rgba[i:i+4]=bytes((col,col,col,round(255*alpha)))
    def chunk(tag,data):
        return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))
    rows=b''.join(b'\0'+rgba[y*width*4:(y+1)*width*4] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))

def make_face_plate(dest):
    # A grayscale albedo projection taken directly from the project-owned
    # death illustration. Directed lighting comes from the game. The modeled
    # eyes, nose, fangs and lips have separate pigments and surface response.
    ref=HERE/'Art/Source/Presentation/wolf_attack.png'
    subprocess.run(['magick',str(ref),'-resize','1138x1040!','-crop','612x636+318+6','+repage',
                    '-background','#58585b','-alpha','remove','-alpha','off','-colorspace','Gray',
                    '-level','3%,79%,1.25','-clamp','-resize','1024x1024!','-depth','8',
                    'PNG24:'+str(dest)],check=True)

def write_nose_texture(path):
    # A periodic cellular relief pattern for the leathery rhinarium. Each cell
    # has a rounded top with narrow furrows separating the scales.
    rng=random.Random(51833)
    size,grid=256,37
    centres={(i,j):(rng.uniform(.19,.81),rng.uniform(.19,.81)) for i in range(grid) for j in range(grid)}
    heights=[]
    for y in range(size):
        for x in range(size):
            u,v=x*grid/size,y*grid/size
            best=[]
            for j in range(int(v)-1,int(v)+2):
                for i in range(int(u)-1,int(u)+2):
                    rx,ry=centres[i%grid,j%grid]
                    best.append((u-rx-i)**2+(v-ry-j)**2)
            a,b=sorted(best)[:2]
            heights.append(clamp((math.sqrt(b)-math.sqrt(a))*4.))
    def h(x,y): return heights[(y%size)*size+x%size]
    rgb=bytearray()
    for y in range(size):
        for x in range(size):
            n=unit((-.24*(h(x+1,y)-h(x-1,y)),.24*(h(x,y+1)-h(x,y-1)),1.))
            rgb.extend(round(127.5*(w+1.)) for w in n)
    subprocess.run(["magick","-size",f"{size}x{size}","-depth","8","rgb:-","PNG24:"+str(path)],input=rgb,check=True)

def export():
    chunks,views,accessors=bytearray(),[],[]
    def stream(rows,typ,ctype,fmt,target=None,bounds=False):
        chunks.extend(bytes(-len(chunks)%4))
        offset=len(chunks)
        for row in rows: chunks.extend(struct.pack("<"+fmt,*row))
        view=dict(buffer=0,byteOffset=offset,byteLength=len(chunks)-offset)
        if target: view["target"]=target
        iv=len(views); views.append(view)
        acc=dict(bufferView=iv,componentType=ctype,count=len(rows),type=typ)
        if bounds: acc.update(min=list(map(min,zip(*rows))),max=list(map(max,zip(*rows))))
        ia=len(accessors); accessors.append(acc)
        return ia
    primitives=[]
    for material,slot in enumerate(SLOTS):
        # Keep glTF sections compact; each material owns its vertex attributes.
        original=list(dict.fromkeys(faces[slot])); remap={old:new for new,old in enumerate(original)}
        attrs={}
        for attr,values,typ,ctype,fmt in (("POSITION",points,"VEC3",5126,"3f"),("NORMAL",normals,"VEC3",5126,"3f"),
            ("COLOR_0",colors,"VEC4",5126,"4f"),("TEXCOORD_0",uvs,"VEC2",5126,"2f"),
            ("TEXCOORD_1",projection,"VEC2",5126,"2f"),("JOINTS_0",ids,"VEC4",5123,"4H"),("WEIGHTS_0",weights,"VEC4",5126,"4f")):
            attrs[attr]=stream([values[i] for i in original],typ,ctype,fmt,34962,attr=="POSITION")
        kind,fmt=(5123,"H") if len(original)<=65535 else (5125,"I")
        index=stream([(remap[i],) for i in faces[slot]],"SCALAR",kind,fmt,34963)
        primitives.append(dict(attributes=attrs,indices=index,material=material))
    nodes,inv=[],[]
    for name,pos,parent in bones:
        x,y,z=xyz(pos)
        local=sub(pos,bones[joints[parent]][1]) if parent else pos
        nodes.append(dict(name=name,translation=xyz(local)))
        if parent: nodes[joints[parent]].setdefault("children",[]).append(joints[name])
        inv.append((1,0,0,0,0,1,0,0,0,0,1,0,-x,-y,-z,1))
    ibm=stream(inv,"MAT4",5126,"16f")
    nodes.append(dict(name="SK_TimberWolf",mesh=0,skin=0))
    data=dict(asset=dict(version="2.0"),scene=0,scenes=[dict(nodes=[0,len(bones)])],nodes=nodes,
        meshes=[dict(name="SK_TimberWolf",primitives=primitives)],materials=[dict(name=slot) for slot in SLOTS],
        skins=[dict(name="TimberWolf",joints=list(range(len(bones))),inverseBindMatrices=ibm,skeleton=0)],
        buffers=[dict(byteLength=len(chunks))],bufferViews=views,accessors=accessors)
    raw=json.dumps(data,separators=(",",":")).encode()
    raw+=b" "*(-len(raw)%4);chunks.extend(bytes(-len(chunks)%4))
    SOURCE.parent.mkdir(parents=True,exist_ok=True)
    SOURCE.write_bytes(struct.pack("<4sII",b"glTF",2,28+len(raw)+len(chunks))+struct.pack("<I4s",len(raw),b"JSON")+raw+
                       struct.pack("<I4s",len(chunks),b"BIN\0")+chunks)
    assert all(abs(sum(ws)-1)<.00002 for ws in weights)
    print(f"{SOURCE.name}: {len(bones)} bones, {len(points)} vertices, {sum(map(len,faces.values()))//3} triangles")
    for slot in SLOTS:
        print(slot,len(faces[slot])//3)
    write_fur_atlas(SOURCE.parent/'TimberGuardHair.png')
    make_face_plate(SOURCE.parent/'TimberFaceMarkings.png')
    write_nose_texture(SOURCE.parent/'TimberNoseLeather.png')

if __name__=="__main__":
    export()
