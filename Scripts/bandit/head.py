"""Sculpt the Villa-inspired head in MetaHuman Creator and bake its game skin."""

from pathlib import Path

import unreal as u
from hermit.appearance import compile_with_renderer, duplicate, save

ROOT = "/Game/NightOfTheWolves/Bandit"
PRESET = "/MetaHumanCharacter/Optional/Presets/Dominic"
MAX = "/Game/NightOfTheWolves/Max/Anatomy"
assets = u.EditorAssetLibrary
edit = u.MaterialEditingLibrary


def sculpt(creator, character):
    marks = creator.get_face_landmarks(character)
    if len(marks) != 90:
        raise RuntimeError("Review the Dominic face landmarks for this engine version.")
    offsets = {}
    # Left and right authored markers use the preset's own index layout.
    # Pull the inner brows together, lower their outside edges and bring the ridge forward.
    for left, right, dx, dy, dz in [
        (76, 23, 0.27, 0.36, -0.25),
        (10, 25, 0.03, 0.24, -0.20),
        (77, 24, 0.15, 0.08, -0.23),
        (43, 19, 0.02, -0.12, -0.11),
        (52, 36, -0.02, 0.10, 0.12),
        # A narrower face at the temples, fuller cheek under the outer eye, broad nose base.
        (70, 74, 0.34, -0.05, 0),
        (57, 40, 0.18, 0.06, -0.18),
        (65, 22, -0.18, 0.55, 0.12),
        (69, 73, -0.08, 0.24, -0.16),
        (66, 44, -0.17, 0.33, -0.23),
        # Taper the lower jaw, give the mouth a straighter line and project the chin.
        (1, 27, 0.52, -0.02, -0.09),
        (2, 28, 0.25, 0.06, -0.14),
        (62, 42, 0.27, 0.10, -0.25),
        (3, 29, 0.04, 0.37, -0.35),
        (47, 31, -0.18, 0.09, 0.05),
    ]:
        if not marks[left].x < 0 < marks[right].x:
            raise RuntimeError("Unexpected face axes at " + str((left, right)))
        offsets[left], offsets[right] = u.Vector(dx, dy, dz), u.Vector(-dx, dy, dz)
    for idx, dy, dz in [
        (58, 0.36, -0.15),
        (6, 0.18, 0.03),
        (63, 0.51, -0.24),
        (64, 0.29, -0.18),
        (61, 0.48, -0.37),
        (7, 0.08, 0.27),
    ]:
        offsets[idx] = u.Vector(0, dy, dz)
    creator.translate_face_landmarks(character, list(offsets), list(offsets.values()))
    creator.commit_face_state(character)


def bake(creator_character):
    scratch = (
        Path(u.Paths.convert_relative_path_to_full(u.Paths.project_intermediate_dir()))
        / "BanditSkin"
    )
    scratch.mkdir(parents=True, exist_ok=True)
    maps = creator_character.get_editor_property("synthesized_face_textures")
    for channel in ("BaseColor", "Normal", "Cavity"):
        name = "T_VillaFace_" + channel
        if assets.does_asset_exist(ROOT + "/" + name):
            continue
        texture = maps[getattr(u.FaceTextureType, channel.upper())]
        image = scratch / (name + ".png")
        options = u.ImageWriteOptions()
        options.format = u.DesiredImageFormat.PNG
        options.overwrite_file = True
        options.set_editor_property("bAsync", False)
        u.ImageWriteBlueprintLibrary.export_to_disk(texture, str(image), options)
        if not image.is_file() or image.stat().st_size < 6000:
            raise RuntimeError("Run the skin bake in a renderer-backed editor.")
        task = u.AssetImportTask()
        task.set_editor_properties(
            {
                "filename": str(image),
                "destination_path": ROOT,
                "destination_name": name,
                "automated": True,
                "replace_existing": True,
            }
        )
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        result = assets.load_asset(ROOT + "/" + name)
        result.set_editor_property("srgb", channel == "BaseColor")
        if channel == "Normal":
            result.set_editor_property(
                "compression_settings", u.TextureCompressionSettings.TC_NORMALMAP
            )
        elif channel == "Cavity":
            result.set_editor_property(
                "compression_settings", u.TextureCompressionSettings.TC_MASKS
            )
        save(result)


def brown_iris(side):
    parent = assets.load_asset("/MetaHumanCharacter/Materials/MI_Eye_" + side + "_MHC")
    name = "MI_VillaEye" + side
    if assets.does_asset_exist(ROOT + "/" + name):
        mat = assets.load_asset(ROOT + "/" + name)
    else:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            ROOT,
            u.MaterialInstanceConstant,
            u.MaterialInstanceConstantFactoryNew(),
        )
    edit.set_material_instance_parent(mat, parent)
    edit.set_material_instance_vector_parameter_value(
        mat, "Iris Color Multiply", u.LinearColor(0.25, 0.14, 0.17, 1)
    )
    edit.set_material_instance_scalar_parameter_value(
        mat, "Iris Global Saturation", 1.16
    )
    edit.set_material_instance_scalar_parameter_value(mat, "Pupil Dilation", 1.12)
    save(mat)
    return mat


def game_material(head, use_baked):
    mat = duplicate(MAX + "/Materials/M_MaxFace", "M_VillaFace", ROOT)
    expressions = list(edit.get_material_expressions(mat))
    vary = next(
        (
            n
            for n in expressions
            if isinstance(n, u.MaterialExpressionVertexInterpolator)
        ),
        None,
    )
    if not vary:
        position = edit.create_material_expression(
            mat, u.MaterialExpressionPreSkinnedPosition
        )
        vary = edit.create_material_expression(
            mat, u.MaterialExpressionVertexInterpolator
        )
        assert edit.connect_material_expressions(position, "", vary, "")
    for n in expressions:
        if isinstance(n, u.MaterialExpressionTextureSample) and use_baked:
            old = n.get_editor_property("texture").get_name()
            for channel in ("BaseColor", "Normal", "Cavity"):
                if old in ("T_Max_Face_" + channel, "T_VillaFace_" + channel):
                    n.set_editor_property(
                        "texture", assets.load_asset(ROOT + "/T_VillaFace_" + channel)
                    )
        if (
            isinstance(n, u.MaterialExpressionCustom)
            and n.get_editor_property("description")
            == "Natural brows, short stubble and hairline"
        ):
            pins = list(n.get_editor_property("inputs"))
            if not any(
                str(pin.get_editor_property("input_name")) == "P" for pin in pins
            ):
                pin = u.CustomInput()
                pin.set_editor_property("input_name", "P")
                n.set_editor_property("inputs", [*pins, pin])
            assert edit.connect_material_expressions(vary, "", n, "P")
            n.set_editor_property(
                "code",
                """
float a=saturate(max(Scalp*1.35, max(pow(Brows,.48)*1.55, max(pow(Beard,.48)*1.14, pow(Lip,.6)*1.10))));
float3 skin=Skin.rgb*float3(1.10,1.04,1.01);
float jaw=smoothstep(151.8,155.,P.z)*(1.-smoothstep(158.0,159.7,P.z-.35*abs(P.x)))*smoothstep(3.,8.5,P.y);
float lips=(1.-smoothstep(2.2,3.4,abs(P.x)))*(1.-smoothstep(.46,1.35,abs(P.z-158.5)));
float2 cell=P.xz*13.+P.y*float2(13.,7.);
float freckle=frac(sin(dot(floor(cell),float2(37.1,73.7)))*2579.3);
float dotHair=(1.-smoothstep(.12,.43,length(frac(cell)-.5)))*freckle;
float shadow=jaw*(1.-lips)*(.31+.37*dotHair);
float brow=exp(-pow((abs(P.x)-(.32+.09*(P.z-168.)))/.14,2))*exp(-pow((P.z-168.6)/1.11,2))*smoothstep(10.0,12.0,P.y);
return lerp(skin*(1.-.19*brow),float3(.020,.014,.011),max(a,shadow));
""",
            )
    # The exported Creator head includes a bare shoulder bib. Keep the neck
    # within the shirt collar and trim the bib in bind space, through every pose.
    trim = edit.get_material_property_input_node(
        mat, u.MaterialProperty.MP_OPACITY_MASK
    )
    if not trim:
        mat.set_editor_property("blend_mode", u.BlendMode.BLEND_MASKED)
        trim = edit.create_material_expression(mat, u.MaterialExpressionCustom)
        pin = u.CustomInput()
        pin.set_editor_property("input_name", "BindPos")
        trim.set_editor_properties(
            {
                "inputs": [pin],
                "description": "Keep the neck inside the campaign collar",
                "output_type": u.CustomMaterialOutputType.CMOT_FLOAT1,
            }
        )
        assert edit.connect_material_expressions(vary, "", trim, "BindPos")
        assert edit.connect_material_property(
            trim, "", u.MaterialProperty.MP_OPACITY_MASK
        )
    # The central throat reaches forward under the chin. The exported skin at
    # 7–9 cm either side belongs to the sloped shoulder, just below the ear.
    # Blend the base of the neck up to the jawline across this transition.
    trim.set_editor_property(
        "code",
        """
float side = abs(BindPos.x);
float edge = 145.0 + 9.85 * smoothstep(4.1, 6.55, side);
edge += 1.4 * smoothstep(6.5, 8.1, side);
edge += 1.0 - smoothstep(-4.0, 1.0, BindPos.y);
return BindPos.z >= edge ? 1.0 : 0.0;
""",
    )
    compile_with_renderer(mat)
    save(mat)
    template = assets.load_asset(MAX + "/Max_Head")
    source = {
        str(slot.material_slot_name): slot.material_interface
        for slot in template.materials
    }
    source["eyeLeft_shader_shader"] = brown_iris("Left")
    source["eyeRight_shader_shader"] = brown_iris("Right")
    slots = list(head.materials)
    for slot in slots:
        name = str(slot.material_slot_name)
        slot.material_interface = (
            mat if name.lower().startswith("head") else source[name]
        )
    head.materials = slots
    save(head)


def hand_material(character):
    path = ROOT + "/MI_VillaBodySkin"
    mat = (
        assets.load_asset(path)
        if assets.does_asset_exist(path)
        else u.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_VillaBodySkin",
            ROOT,
            u.MaterialInstanceConstant,
            u.MaterialInstanceConstantFactoryNew(),
        )
    )
    edit.set_material_instance_parent(
        mat, assets.load_asset(MAX + "/Materials/M_MaxBody")
    )
    for name in ("Bias", "Gain"):
        c = character.skin_settings.skin.get_editor_property("body_" + name.lower())
        edit.set_material_instance_vector_parameter_value(
            mat, "Skin " + name, u.LinearColor(c.x, c.y, c.z, 1)
        )
    save(mat)


def build(render_skin):
    path = ROOT + "/Creator/Villa"
    fresh = not assets.does_asset_exist(path)
    character = duplicate(PRESET, "Villa", ROOT + "/Creator")
    c = u.get_editor_subsystem(u.MetaHumanCharacterEditorSubsystem)
    if not c.try_add_object_to_edit(character):
        raise RuntimeError("Could not open the bandit's Creator asset.")
    try:
        if fresh:
            character.get_editor_property("assembly_settings").set_editor_property(
                "name_override", "Villa"
            )
            sculpt(c, character)
            save(character)
        if not assets.does_asset_exist(ROOT + "/Villa_Head"):
            export = u.MetaHumanGeometryExportParams()
            export.project_path = ROOT
            export.head_skeletal_mesh = True
            export.body_skeletal_mesh = False
            export.full_body_skeletal_mesh = False
            u.MetaHumanCharacterExportBlueprintLibrary.export_geometry(
                character, export
            )
        hand_material(character)
        if render_skin:
            bake(character)
        game_material(
            assets.load_asset(ROOT + "/Villa_Head"),
            assets.does_asset_exist(ROOT + "/T_VillaFace_BaseColor"),
        )
    finally:
        c.remove_object_to_edit(character)
