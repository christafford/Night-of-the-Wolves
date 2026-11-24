"""Prepare the scans and model the two-tie bale. Run with Python 3 and Pillow."""
from io import BytesIO
import hashlib
import json
import math
from pathlib import Path
import random
import struct
from urllib.request import Request, urlopen
import zipfile

from PIL import Image

DEST = Path(__file__).resolve().parents[2] / 'Art/Source/FirstLeg/CottageYard'
AGENT = 'Night-of-the-Wolves-asset-authoring'
HAY_URL = 'https://www.texturecan.com/downloads/ground_0018/ground_0018_1k_BQmh7j.zip'


def fetch(url):
    with urlopen(Request(url, headers={'User-Agent': AGENT}), timeout=90) as response:
        return response.read()


def download(record):
    contents = fetch(record['url'])
    if hashlib.md5(contents).hexdigest() != record['md5']:
        raise ValueError('Unexpected file hash for ' + record['url'])
    return contents


def write_glb(name, doc, binary):
    doc.update(asset={'version': '2.0'}, buffers=[{'byteLength': len(binary)}], scene=0,
               scenes=[{'nodes': [0]}], nodes=[{'name': name, 'mesh': 0}])
    header = json.dumps(doc, separators=(',', ':')).encode()
    header += b' ' * (-len(header) % 4)
    binary += b'\0' * (-len(binary) % 4)
    with (DEST / (name + '.glb')).open('wb') as output:
        output.write(struct.pack('<4sII', b'glTF', 2, 28 + len(header) + len(binary)))
        output.write(struct.pack('<I4s', len(header), b'JSON') + header)
        output.write(struct.pack('<I4s', len(binary), b'BIN\0') + binary)


def scan(asset, name, material, grip_height=0., scale=1.):
    meta = json.loads(fetch('https://api.polyhaven.com/files/' + asset))
    ref = meta['gltf']['1k']['gltf']
    original = json.loads(download(ref))
    blob = bytearray(download(ref['include'][original['buffers'][0]['uri']]))
    # The barrel lid, bung, bottom and staves share an origin, one UV set and one material.
    primitives = []
    for node in original['nodes']:
        if any(key in node for key in ('translation', 'rotation', 'scale', 'matrix')):
            raise ValueError('Source transform changed')
        for prim in original['meshes'][node['mesh']]['primitives']:
            primitives.append(prim)
            acc = original['accessors'][prim['attributes']['POSITION']]
            view = original['bufferViews'][acc['bufferView']]
            offset = view.get('byteOffset', 0) + acc.get('byteOffset', 0)
            for i in range(acc['count']):
                at = offset + i * view.get('byteStride', 12)
                x, y, z = struct.unpack_from('<fff', blob, at)
                struct.pack_into('<fff', blob, at, scale*x, scale*(y-grip_height), scale*z)
            for key in ('min', 'max'):
                x, y, z = acc[key]
                acc[key] = [scale*x, scale*(y-grip_height), scale*z]
    doc = {k: original[k] for k in ('bufferViews', 'accessors')}
    doc.update(meshes=[{'name': name, 'primitives': primitives}], materials=[{'name': material}])
    write_glb(name, doc, blob)
    for src, suffix in [('Diffuse', 'Color'), ('nor_dx', 'Normal'), ('arm', 'ORM')]:
        (DEST / f'T_{material}_{suffix}.jpg').write_bytes(download(meta[src]['1k']['jpg']))


def bale_textures():
    with zipfile.ZipFile(BytesIO(fetch(HAY_URL))) as archive:
        (DEST / 'T_DryHay_Color.jpg').write_bytes(archive.read('ground_0018_color_1k.jpg'))
        (DEST / 'T_DryHay_Normal.png').write_bytes(archive.read('ground_0018_normal_directx_1k.png'))
        ao = Image.open(BytesIO(archive.read('ground_0018_ao_1k.jpg'))).convert('L')
        rough = Image.open(BytesIO(archive.read('ground_0018_roughness_1k.jpg'))).convert('L')
        Image.merge('RGB', (ao, rough, Image.new('L', ao.size))).save(DEST / 'T_DryHay_ORM.jpg', quality=95)


def bale():
    rng = random.Random(236)
    binary = bytearray()
    views, accessors, primitives = [], [], []

    def stream(rows, shape, fmt, enum):
        offset = len(binary)
        pack = struct.Struct('<' + fmt * len(rows[0]))
        for row in rows:
            binary.extend(pack.pack(*row))
        views.append(dict(buffer=0, byteOffset=offset, byteLength=len(binary)-offset))
        binary.extend(b'\0' * (-len(binary) % 4))
        ac = dict(bufferView=len(views)-1, componentType=enum, count=len(rows), type=shape)
        if shape == 'VEC3':
            ac.update(min=list(map(min, zip(*rows))), max=list(map(max, zip(*rows))))
        accessors.append(ac)
        return len(accessors)-1

    pos, uv, indices, colors = [], [], [], []

    def vert(p, tex=(0., 0.), color=(1., 1., 1.)):
        pos.append(p)
        uv.append(tex)
        colors.append(color)
        return len(pos)-1

    def quad(a, b, c, d):
        indices.extend([(a,), (b,), (c,), (a,), (c,), (d,)])

    def finish(material):
        primitives.append({'attributes': {'POSITION': stream(pos, 'VEC3', 'f', 5126),
                                          'TEXCOORD_0': stream(uv, 'VEC2', 'f', 5126),
                                          'COLOR_0': stream(colors, 'VEC3', 'f', 5126)},
                           'indices': stream(indices, 'SCALAR', 'I', 5125), 'material': material})
        pos.clear(); uv.clear(); colors.clear(); indices.clear()

    def signed(x, power):
        return math.copysign(abs(x)**power, x)

    def surface(x, angle):
        # Broad faces, softened corners, and grooves where the two ties bite into the hay.
        pinch = .040 * math.exp(-((abs(x)-.245)/.044)**2)
        end = .12 * max(0., (abs(x)-.425)/.063)**2
        r = 1. - pinch - end + .012*math.sin(35*x+4*angle) + .009*math.sin(47*x-5*angle)
        return (x, .244 + .238*r*signed(math.cos(angle), .38),
                .269*r*signed(math.sin(angle), .38))

    count = 36
    rows = []
    for j in range(37):
        x = -.49 + j * .98 / 36
        row = []
        for i in range(count+1):
            row.append(vert(surface(x, math.tau*i/count), (x*1.7+.9, 3.0*i/count)))
        rows.append(row)
    for row, nxt in zip(rows, rows[1:]):
        for i in range(count):
            quad(row[i], row[i+1], nxt[i+1], nxt[i])
    for side in (-1, 1):
        p = surface(.49*side, 0)
        mid = vert((p[0], .244, 0), (.5, .5))
        ring = []
        for i in range(count):
            p = surface(.49*side, math.tau*i/count)
            ring.append(vert(p, (.5+p[2]*1.7, .5+(p[1]-.244)*1.7)))
        for i in range(count):
            pair = (ring[i], ring[(i+1)%count])
            indices.extend((v,) for v in (mid, *(pair if side>0 else pair[::-1])))
    finish(0)

    def sub(a,b): return tuple(x-y for x,y in zip(a,b))
    def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    def norm(v):
        l = math.sqrt(sum(x*x for x in v))
        return tuple(x/l for x in v)

    def strand(points, radii, color, facets):
        rings = []
        for i, p in enumerate(points):
            direction = norm(sub(points[min(i+1,len(points)-1)], points[max(i-1,0)]))
            u = norm(cross(direction, (0,1,0) if abs(direction[1])<.9 else (1,0,0)))
            v = cross(direction,u)
            row = []
            for k in range(facets):
                a = math.tau*k/facets
                row.append(vert(tuple(p[h] + radii[i]*(math.cos(a)*u[h]+math.sin(a)*v[h]) for h in range(3)), color=color))
            rings.append(row)
        for row,nxt in zip(rings,rings[1:]):
            for i in range(facets):
                k=(i+1)%facets
                quad(row[i], row[k], nxt[k], nxt[i])

    for side in (-1, 1):
        # Short broken stems at the cut ends. Longer bent stalks loosen along the sides.
        for i in range(390):
            x=.485*side; y=rng.uniform(.033,.453); z=rng.uniform(-.222,.222)
            tip=x+side*rng.uniform(.006,.044)
            twig=[(x-.012*side,y,z),(x+.003*side,y,z),(tip,max(.005,y+rng.uniform(-.021,.014)),z+rng.uniform(-.025,.025))]
            shade=rng.uniform(.45,1.)
            strand(twig,[.0013,.0018,.00065],(.57*shade,.36*shade,.119*shade),3)
    for i in range(580):
        x=rng.uniform(-.43,.43); a=rng.uniform(0,math.tau); length=rng.uniform(.025,.16)
        tip=min(.52,x+length)
        points=[]
        for k in range(3):
            t=k*.5
            px,py,pz=surface(x+(tip-x)*t, a+.048*math.sin(7*i)*t)
            lift=.002+t*t*.022
            points.append((px,max(.003,py+lift*math.cos(a)),pz+lift*math.sin(a)))
        shade=rng.uniform(.48,1.)
        strand(points,[.0009,.0019,.00065],(.58*shade,.39*shade,.153*shade),3)

    # Each binding is two spiraled strands, with a simple tied loop and frayed ends on top.
    for x in (-.245,.245):
        for phase, color in [(0,(.27,.16,.063)),(math.pi,(.43,.28,.115))]:
            points=[]
            for i in range(257):
                a=math.tau*i/256
                xx,y,z=surface(x,a)
                twist=phase+29*a
                inward=.0018*math.sin(twist)+.003
                points.append((xx+.0025*math.cos(twist),y+math.cos(a)*inward,z+math.sin(a)*inward))
            strand(points,[.0029]*len(points),color,5)
        _,y,z=surface(x,.23)
        knot=[(x-.006,y+.004,z-.014),(x+.014,y+.017,z),(x-.012,y+.019,z+.013),
              (x-.010,y+.007,z-.003),(x+.016,y+.011,z-.012)]
        strand(knot,[.0038]*len(knot),(.35,.22,.09),5)
        strand([knot[-1],(x+.038,y+.018,z-.018),(x+.066,y+.013,z-.026)],
               [.0034,.0028,.0012],(.46,.31,.13),5)
    finish(1)
    write_glb('SM_BoundHay', {'bufferViews':views,'accessors':accessors,'materials':[{'name':'DryHay'},{'name':'FieldFibres'}],
                              'meshes':[{'name':'SM_BoundHay','primitives':primitives}]}, binary)


if __name__ == '__main__':
    DEST.mkdir(parents=True, exist_ok=True)
    scan('rusted_spade_01', 'SM_FieldSpade', 'FieldSpade', grip_height=.123, scale=1.12)
    scan('wine_barrel_01', 'SM_OakBarrel', 'OakBarrel')
    bale_textures()
    bale()
