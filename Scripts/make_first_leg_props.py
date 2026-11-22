"""Author the pack silhouette, hand props and original Foley. Coordinates are Unreal cm."""
import json
import math
from pathlib import Path
import random
import struct
import wave

DEST = Path(__file__).resolve().parents[1] / "Art" / "Source" / "FirstLeg"
DEST.mkdir(parents=True, exist_ok=True)
rng = random.Random(4925)
tau = 2 * math.pi

def subtract(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def unit(a):
    length = math.sqrt(dot(a,a))
    return tuple(x/length for x in a) if length else (0,0,1)

class Mesh:
    def __init__(self):
        self.vertices, self.normals, self.colors = [], [], []

    def tri(self,a,b,c,color,reference):
        normal = cross(subtract(b,a),subtract(c,a))
        if dot(normal,normal)<1e-9: return
        if dot(normal,subtract(a,reference))<0:
            b,c = c,b
            normal = tuple(-v for v in normal)
        x,y,z = unit(normal)
        shade = rng.uniform(.90,1.09)
        for px,py,pz in (a,c,b):  # reverse winding when converting coordinate handedness
            self.vertices.append((px*.01,pz*.01,py*.01))
            self.normals.append((x,z,y))
            self.colors.append(tuple(min(1,t*shade) for t in color))

    def rings(self,sections,color,n=10,axis='x'):
        # Each section carries its center and two radii.
        rows=[]
        for x,y,z,u,v in sections:
            row=[]
            for i in range(n):
                a=tau*i/n
                du,dv=u*math.cos(a),v*math.sin(a)
                row.append((x,y+du,z+dv) if axis=='x' else (x+du,y+dv,z))
            rows.append(row)
        for j,(r,s) in enumerate(zip(rows,rows[1:])):
            middle=tuple((a+b)*.5 for a,b in zip(sections[j][:3],sections[j+1][:3]))
            for i in range(n):
                k=(i+1)%n
                self.tri(r[i],s[i],r[k],color,middle)
                self.tri(r[k],s[i],s[k],color,middle)
        for j,other in [(0,1),(-1,-2)]:
            for i in range(n):
                self.tri(sections[j][:3],rows[j][i],rows[j][(i+1)%n],color,sections[other][:3])

    def export(self,name):
        vertices=[*zip(*self.vertices)]
        chunks=[b''.join(struct.pack('<3f',*v) for v in values) for values in (self.vertices,self.normals,self.colors)]
        size=len(chunks[0]); total=size*3
        gltf={'asset':{'version':'2.0'},'scene':0,'scenes':[{'nodes':[0]}],'nodes':[{'mesh':0,'name':name}],
              'meshes':[{'name':name,'primitives':[{'attributes':{'POSITION':0,'NORMAL':1,'COLOR_0':2}}]}],
              'buffers':[{'byteLength':total}],
              'bufferViews':[{'buffer':0,'byteOffset':size*i,'byteLength':size,'target':34962} for i in range(3)],
              'accessors':[{'bufferView':i,'componentType':5126,'count':len(self.vertices),'type':'VEC3'} for i in range(3)]}
        gltf['accessors'][0].update(min=[min(v) for v in vertices],max=[max(v) for v in vertices])
        js=json.dumps(gltf).encode(); js+=b' '*(-len(js)%4)
        (DEST/(name+'.glb')).write_bytes(struct.pack('<4sII',b'glTF',2,28+len(js)+total)+struct.pack('<I4s',len(js),b'JSON')+js+struct.pack('<I4s',total,b'BIN\x00')+b''.join(chunks))

def wolf():
    m=Mesh(); fur=(.15,.155,.15); pale=(.31,.29,.25); black=(.035,.032,.03)
    # Long ribcage, tucked belly and ruff, wedge skull with a distinct muzzle.
    m.rings([(-52,0,51,4,7),(-41,0,51,14,19),(-23,0,55,11,14),(-5,0,53,15,18),(18,0,55,18,24),(33,0,62,16,20),(45,0,67,9,12)],fur)
    m.rings([(24,0,62,14,18),(35,0,72,17,18),(48,0,76,12,12),(58,0,73,9,8),(72,0,70,5.5,4.7)],pale)
    m.rings([(69,0,71,5.6,4.7),(74,0,71,5,3.5)],black)
    for side in (-1,1):
        m.rings([(42,side*8,82,7,5),(39,side*10,92,4,2.6),(38,side*11,99,.35,.2)],fur,7,'z')
        m.rings([(51,side*10.2,79,2.4,1.3),(54,side*10.5,79.8,1.3,1.1)],(.70,.19,.045),7)
        for x,knee,hock in [(24,19,21),(-36,-22,-40)]:
            shift=side*(4 if x>0 else -5)
            m.rings([(x,side*11,58,10,6),(x,side*12,39,8,5),(knee+shift,side*13,25,5,4),(hock+shift,side*13,12,3,3),(hock+shift+2,side*13,5,2.5,3)],fur,8,'z')
            m.rings([(hock+shift+5,side*13,1,7,4),(hock+shift+3,side*13,5,5,4),(hock+shift+1,side*13,7,3,3)],pale,8,'z')
    m.rings([(-88,4,20,.4,1),(-80,3,22,6,7),(-68,2,28,9,10),(-59,1,41,8,10),(-48,0,55,5,7)],fur)
    m.export('SM_TimberWolf')

def props():
    wood=(.32,.19,.08); m=Mesh()
    m.rings([(0,0,-19,2.2,2.4),(0,0,0,2,2),(0,0,38,2.7,2.5),(2,0,59,4,3),(4,0,65,2.5,2)],wood,8,'z'); m.export('Firebrand')
    m=Mesh()
    m.rings([(0,0,-47,2.2,2.2),(0,0,29,1.9,1.9)],wood,9,'z')
    m.rings([(0,0,-43,2,3),(1,0,-50,2,10),(2,0,-63,1.5,9),(2.5,0,-70,.6,1)],(.20,.24,.24),8,'z'); m.export('Shovel')
    m=Mesh()
    for j in range(5):
        for i in range(36):
            a,b=tau*i/36,tau*(i+1)/36
            def center(t): return (math.cos(t)*10,1.5*j+.7*math.sin(t*2+j),math.sin(t)*15)
            def ring(t,k):
                x,y,z=center(t); r=tau*k/6
                return (x+1.15*math.cos(r)*math.cos(t),y+1.15*math.sin(r),z+1.15*math.cos(r)*math.sin(t))
            ref=center((a+b)*.5)
            for k in range(6):
                m.tri(ring(a,k),ring(b,k),ring(b,k+1),(.39,.29,.16),ref)
                m.tri(ring(a,k),ring(b,k+1),ring(a,k+1),(.39,.29,.16),ref)
    m.export('Rope')
    m=Mesh()
    m.rings([(0,0,-16,3,3),(0,0,-13,8,7),(0,0,-7,9,7),(0,0,0,3,3),(0,0,4,5,4)],(.30,.15,.055),12,'z')
    m.rings([(0,0,-.6,3.8,3.5),(0,0,.8,3.8,3.5)],(.72,.46,.10),12,'z'); m.export('Gold')

def foley(name,duration,fn):
    rate=24000; samples=bytearray(); low=0.; slow=0.
    for i in range(int(duration*rate)):
        t=i/rate; white=rng.uniform(-1.,1.); low+=.11*(white-low); slow+=.014*(white-slow)
        sample=fn(t,white,low,slow)*min(t*450,1,(duration-t)*120)
        samples.extend(struct.pack('<h',int(max(-1,min(1,sample))*32000)))
    with wave.open(str(DEST/(name+'.wav')),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','not compressed')); w.writeframes(samples)

if __name__=='__main__':
    wolf(); props()
    foley('Shot',1.15,lambda t,w,l,s: (l*2.3+w*.3+math.sin(tau*t*(75+95*math.exp(-30*t)))*.5)*math.exp(-11*t)+l*.5*math.exp(-6*abs(t-.24)))
    foley('DoorImpact',.85,lambda t,w,l,s: (.4*math.sin(tau*76*t)+.3*math.sin(tau*112*t)+l)*math.exp(-10*t)+w*.15*math.exp(-26*abs(t-.20)))
    foley('Growl',1.4,lambda t,w,l,s: (.18*math.sin(tau*(49*t+1.25*math.sin(11*t)))+.17*l)*(.4+.6*math.sin(tau*21*t)**2)*math.sin(math.pi*t/1.4)**.5)
    foley('Hearth',5,lambda t,w,l,s: .45*s+.065*l+(w*.19 if rng.random()<.0025 else 0))
