"""Build the two eroded creek banks and the wooded rise beyond the bandit's trail."""

import math
import sys
from pathlib import Path

import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_cascades import DEST, load_or_import

assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
road = [
    (10970, 50),
    (11900, 50),
    (12800, 45),
    (13450, 47),
    (14050, 50),
    (14700, 50),
    (15600, 55),
    (16400, 80),
    (17200, 95),
    (18600, 130),
]


BANK_HLSL = """
float BankHeight = W.z + 14*sin(W.y*.017)+9*sin(W.y*.034+W.x*.019);
float3 gravel = Stone * float3(.78,.77,.73);
float3 clay = lerp(Mud * float3(.77,.66,.51),gravel,.24);
float3 ForestDuff = Duff * float3(.58,.64,.51);
return lerp(lerp(gravel,clay,smoothstep(-170,-68,BankHeight)),ForestDuff,smoothstep(-32,44,BankHeight));"""


def material():
    path = DEST + "/M_CreekBank"
    if assets.does_asset_exist(path):
        mat = assets.load_asset(path)
        blend = edit.get_material_property_input_node(mat, u.MaterialProperty.MP_BASE_COLOR)
        if not isinstance(blend, u.MaterialExpressionCustom):
            raise RuntimeError("The creek surface material needs its landcover blend node.")
        blend.set_editor_property("code", BANK_HLSL)
        assets.save_loaded_asset(mat, False)
        return mat
    else:
        mat = tools.create_asset("M_CreekBank", DEST, u.Material, u.MaterialFactoryNew())
    mat.set_editor_properties({"used_with_instanced_static_meshes": True, "used_with_nanite": True})

    def node(kind, x, y):
        return edit.create_material_expression(mat, kind, x, y)

    def sample(texture, y):
        n = node(u.MaterialExpressionTextureSample, -750, y)
        n.set_editor_property("texture", texture)
        return n

    grit = sample(load_or_import("dry_riverbed_rock_Diffuse.jpg"), 0)
    norm = load_or_import("dry_riverbed_rock_nor_dx.jpg")
    norm.set_editor_properties(
        {"compression_settings": u.TextureCompressionSettings.TC_NORMALMAP, "srgb": False}
    )
    assets.save_loaded_asset(norm)
    detail = sample(norm, 290)
    detail.set_editor_property("sampler_type", u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    litter = sample(assets.load_asset("/Game/NightOfTheWolves/Presentation/forest_ground_04"), -440)
    earth = sample(assets.load_asset("/Game/NightOfTheWolves/Presentation/mud_forest"), -210)
    w = node(u.MaterialExpressionWorldPosition, -710, 510)
    blend = node(u.MaterialExpressionCustom, -100, -90)
    pins = [(grit, "RGB", "Stone"), (litter, "RGB", "Duff"), (earth, "RGB", "Mud"), (w, "", "W")]
    inp = []
    for _, _, name in pins:
        item = u.CustomInput()
        item.set_editor_property("input_name", name)
        inp.append(item)
    blend.set_editor_properties(
        {
            "description": "Shingle at the high-water line, exposed soil above, duff at the lip",
            "output_type": u.CustomMaterialOutputType.CMOT_FLOAT3,
            "inputs": inp,
            "code": BANK_HLSL,
        }
    )
    for obj, out, pin in pins:
        if not edit.connect_material_expressions(obj, out, blend, pin):
            raise RuntimeError("Missing material pin " + pin)
    edit.connect_material_property(blend, "", u.MaterialProperty.MP_BASE_COLOR)
    edit.connect_material_property(detail, "RGB", u.MaterialProperty.MP_NORMAL)
    rough = node(u.MaterialExpressionConstant, -160, 390)
    rough.set_editor_property("r", 0.86)
    edit.connect_material_property(rough, "", u.MaterialProperty.MP_ROUGHNESS)
    assets.save_loaded_asset(mat)
    return mat


def ground(x, y):
    for (a, za), (b, zb) in zip(road[:-1], road[1:]):
        if x <= b:
            z = za + max(0, (x - a) / (b - a)) * (zb - za)
            break
    else:
        z = 130
    side = max(0.0, abs(y - 825) - 2350) / 1800
    end = max(0.0, x - 18150) / 1250
    hills = min(1.65, max(side, end))
    return (
        z
        - 8
        + hills
        * hills
        * (360 + 76 * math.sin(y * 0.0027 + x * 0.0014) + 60 * math.cos(y * 0.0019 - x * 0.0021))
    )


def geometry(mat):
    verts, uvs, faces = [], [], []
    for sign in (-1, 1):
        offsets = [-245, -193, -140, -95, -58, -28, 0, 35, 90, 180, 310, 490, 750, 1100, 1500, 1880]
        if sign == 1:
            offsets += list(range(2230, 12800, 375))
        for i, y in enumerate(range(-7400, 6500, 100)):
            t = (y + 1100) / 1650
            rim = (9800 if sign == -1 else 10900) + 150 * math.sin(t) + 35 * math.sin(2.4 * t)
            # Broad toes under smaller cutbanks vary independently of the playable lip.
            roll = (
                math.sin(y * 0.011 + sign) * 0.45
                + math.cos(y * 0.021) * 0.21
                + math.sin(y * 0.0038) * 0.55
            )
            row = []
            for j, off in enumerate(offsets):
                ease = min(1.0, max(0.0, (off + 245) / 250))
                x = rim + sign * (
                    off
                    + (1 - ease) * roll * 44
                    + ease * min(1, abs(off) / 150) * 11 * math.sin(y * 0.012 + off * 0.009)
                )
                top = ground(x, y)
                if off < 35:
                    height = -255 + (top + 255) * (1 - (1 - ease) ** 1.65)
                    height += math.sin(ease * math.pi) * (
                        22 * roll + 10 * math.sin(y * 0.027 + off * 0.044)
                    )
                else:
                    height = top
                dist = math.hypot(x - row[-1][0], height - row[-1][1]) if row else 0
                uvx = row[-1][2] + dist if row else 0
                row.append((x, height, uvx))
                verts.append(u.Vector(x, y, height))
                uvs.append(u.Vector2D(uvx / 190, y / 190))
                if i and j:
                    d = len(verts) - 1
                    a, b, c = d - len(offsets) - 1, d - len(offsets), d - 1
                    for tri in ((a, b, d), (a, d, c)):
                        faces.append(u.IntVector(*(tri[::-1] if sign == 1 else tri)))
    buffers = u.GeometryScriptSimpleMeshBuffers()
    buffers.set_editor_properties({"vertices": verts, "uv0": uvs, "triangles": faces})
    mesh = u.DynamicMesh()
    mesh.append_buffers_to_mesh(buffers)
    path = DEST + "/SM_CreekValley"
    if assets.does_asset_exist(path):
        target = assets.load_asset(path)
        opts = u.GeometryScriptCopyMeshToAssetOptions()
        opts.set_editor_properties({"enable_recompute_normals": True, "enable_recompute_tangents": True})
        _, outcome = u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(
            mesh, target, opts, u.GeometryScriptMeshWriteLOD()
        )
    else:
        opts = u.GeometryScriptCreateNewStaticMeshAssetOptions()
        opts.set_editor_properties(
            {
                "enable_recompute_normals": True,
                "enable_recompute_tangents": True,
                "enable_collision": False,
            }
        )
        target, outcome = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(
            mesh, path, opts
        )
    if outcome != u.GeometryScriptOutcomePins.SUCCESS:
        raise RuntimeError(str(outcome))
    target.set_material(0, mat)
    assets.save_loaded_asset(target, False)
    u.log("CREEK_VALLEY_BUILT %s triangles %s bounds" % (len(faces), target.get_bounds()))


if __name__ == "__main__":
    geometry(material())
