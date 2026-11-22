"""World-scale forest floor and feathered dirt path for the spline blockout meshes."""

import unreal as u

DEST = "/Game/NightOfTheWolves/Cascades"
PRESENTATION = "/Game/NightOfTheWolves/Presentation"
edit = u.MaterialEditingLibrary
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()


def build(name, trail):
    if assets.does_asset_exist(DEST + "/" + name):
        mat = assets.load_asset(DEST + "/" + name)
        edit.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(name, DEST, u.Material, u.MaterialFactoryNew())

    def node(kind, x, y):
        return edit.create_material_expression(mat, kind, x, y)

    def connect(a, out, b, pin):
        if not edit.connect_material_expressions(a, out, b, pin):
            raise RuntimeError("Missing material pin " + pin)

    world = node(u.MaterialExpressionWorldPosition, -1350, -400)
    xy = node(u.MaterialExpressionComponentMask, -1150, -400)
    xy.set_editor_properties({"r": True, "g": True, "b": False, "a": False})
    connect(world, "", xy, "")

    def sample(asset, scale, y):
        uv = node(u.MaterialExpressionMultiply, -950, y)
        uv.set_editor_property("const_b", scale)
        connect(xy, "", uv, "A")
        tex = node(u.MaterialExpressionTextureSample, -720, y)
        tex.set_editor_property("texture", assets.load_asset(PRESENTATION + "/" + asset))
        connect(uv, "", tex, "UVs")
        return tex

    leaf = sample("forest_ground_04", 0.0048, -400)
    dirt = sample("mud_forest", 0.0055, -150)
    stone = sample("grey_stone_path", 0.0048, 100)
    custom = node(u.MaterialExpressionCustom, -180, 0)
    pins = ["Forest", "Dirt", "Gravel", "W"] + (["Local"] if trail else [])
    inputs = []
    for pin in pins:
        entry = u.CustomInput()
        entry.set_editor_property("input_name", pin)
        inputs.append(entry)
    custom.set_editor_property("inputs", inputs)
    custom.set_editor_property("description", "Needle litter and damp trail")
    custom.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3)
    code = """
float broad = sin(W.x*.0013+W.y*.0018)*.5 + sin(W.y*.0067-W.x*.0035)*.27;
float moss = saturate(.28+broad*.48);
float3 litter = Forest * lerp(float3(.66,.69,.59),float3(.48,.64,.37),moss);
float3 soil = Dirt*float3(1.05,.93,.77);
float gravel = smoothstep(.18,.42,Gravel.r)*.32;
float3 tread = lerp(soil,Gravel*float3(.68,.63,.51),gravel);
"""
    if trail:
        local = node(u.MaterialExpressionTransformPosition, -900, 320)
        local.set_editor_properties(
            {
                "transform_source_type": u.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,
                "transform_type": u.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL,
            }
        )
        connect(world, "", local, "Input")
        code += "float edge=abs(Local.y + broad*6)+ (Forest.r-Dirt.r)*15; return lerp(tread,litter,smoothstep(12,29,edge));"
    else:
        code += "return litter;"
    custom.set_editor_property("code", code)
    for n, s, t in [
        (leaf, "RGB", "Forest"),
        (dirt, "RGB", "Dirt"),
        (stone, "RGB", "Gravel"),
        (world, "", "W"),
    ]:
        connect(n, s, custom, t)
    if trail:
        connect(local, "", custom, "Local")
    edit.connect_material_property(custom, "", u.MaterialProperty.MP_BASE_COLOR)
    rough = node(u.MaterialExpressionConstant, -160, 240)
    rough.set_editor_property("r", 0.87)
    edit.connect_material_property(rough, "", u.MaterialProperty.MP_ROUGHNESS)
    assets.save_loaded_asset(mat)
    return mat


if __name__ == "__main__":
    build("M_DampTread", True)
    build("M_NeedleLitter", False)
