"""Assign the Creator tooth, gum and tongue maps to the two speaking heads."""
import unreal as u

root = "/Game/NightOfTheWolves/FirstLeg"
name = "M_DialogueMouth"
textures = "/MetaHumanCharacter/Lookdev_UHM/Teeth/Textures/"
assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary
material = assets.load_asset(root + "/" + name) if assets.does_asset_exist(root + "/" + name) else (
    u.AssetToolsHelpers.get_asset_tools().create_asset(name, root, u.Material, u.MaterialFactoryNew())
)
material.set_editor_property("used_with_skeletal_mesh", True)
edit.delete_all_material_expressions(material)


def node(kind, x, y, **properties):
    expression = edit.create_material_expression(material, kind, x, y)
    expression.set_editor_properties(properties)
    return expression


def sample(name, kind, y):
    return node(u.MaterialExpressionTextureSample, -750, y, texture=u.load_asset(textures + name), sampler_type=kind)


albedo = sample("T_Teeth_BaseColor", u.MaterialSamplerType.SAMPLERTYPE_COLOR, -220)
normal = sample("T_Teeth_Normal", u.MaterialSamplerType.SAMPLERTYPE_NORMAL, 340)
position = node(u.MaterialExpressionPreSkinnedPosition, -980, 80)
vary = node(u.MaterialExpressionVertexInterpolator, -750, 80)
assert edit.connect_material_expressions(position, "", vary, "")
pins = []
for pin_name in ("Albedo", "BindPos"):
    pin = u.CustomInput()
    pin.set_editor_property("input_name", pin_name)
    pins.append(pin)
color = node(u.MaterialExpressionCustom, -370, -120, description="Occlusion behind lips",
             output_type=u.CustomMaterialOutputType.CMOT_FLOAT3, inputs=pins,
             code="return Albedo * lerp(.12, .55, smoothstep(4.0, 12.6, BindPos.y));")
assert edit.connect_material_expressions(albedo, "RGB", color, "Albedo")
assert edit.connect_material_expressions(vary, "", color, "BindPos")
assert edit.connect_material_property(color, "", u.MaterialProperty.MP_BASE_COLOR)
assert edit.connect_material_property(normal, "RGB", u.MaterialProperty.MP_NORMAL)
for i, (pin, value) in enumerate([(u.MaterialProperty.MP_SPECULAR, .24), (u.MaterialProperty.MP_ROUGHNESS, .57)]):
    assert edit.connect_material_property(node(u.MaterialExpressionConstant, -360, 240 + i * 120, r=value), "", pin)
errors = edit.recompile_material(material)
if errors:
    raise RuntimeError(str(errors))
assert assets.save_loaded_asset(material, False)
for path in [root + "/Hermit/Hermit_Head", "/Game/NightOfTheWolves/Bandit/Villa_Head"]:
    head = u.load_asset(path)
    slots = list(head.materials)
    teeth = next(slot for slot in slots if str(slot.material_slot_name).startswith("teeth_"))
    teeth.material_interface = material
    head.materials = slots
    assert assets.save_loaded_asset(head, False)
    u.log(f"DIALOGUE MOUTH {head.get_name()} {material.get_path_name()}")
