"""Create Max with the installed MetaHuman Creator. Run in the Unreal editor Python environment."""
from pathlib import Path

import unreal

ROOT = "/Game/NightOfTheWolves/Max"
CHARACTER = ROOT + "/Creator/Max"
ANATOMY = ROOT + "/Anatomy"
PRESET = "/MetaHumanCharacter/Optional/Presets/Mateo"
GROOMS = "/MetaHumanCharacter/Optional/Grooms/Bindings/"
ASSETS = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
CREATOR = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)


def select_grooms(character):
    collection = character.internal_collection
    instance = collection.default_instance
    for slot, path in {
        "Hair": "Hair/WI_Hair_S_SideSweptFringe",
        "Eyebrows": "Eyebrows/WI_Eyebrows_M_Natural",
        "Beard": "Beards/WI_Beard_S_Stubble",
        "Mustache": "Mustaches/WI_Mustache_S_Stubble",
    }.items():
        item = unreal.load_asset(GROOMS + path)
        key = collection.try_add_item_from_wardrobe_item(slot, item)
        if not key:
            raise RuntimeError("Could not select " + path)
        instance.set_single_slot_selection(slot, key)


def sculpt(character):
    # The 5.9 face rig uses X across the face, Y forward, and Z up (centimetres).
    # Move mirrored lower jaw and nostril anchors inward, keep a subtle straight bridge,
    # and give the chin a small amount of forward projection.
    points = CREATOR.get_face_landmarks(character)
    if len(points) != 85:
        raise RuntimeError("Check the landmark indices against this version of the face rig.")
    moves = {}
    for left, right, amount in [(1, 28, 0.25), (2, 29, 0.22), (63, 43, 0.14), (67, 45, 0.12)]:
        if not points[left].x < 0 < points[right].x:
            raise RuntimeError("Unexpected face landmark coordinate system.")
        moves[left] = unreal.Vector(amount, 0, 0)
        moves[right] = unreal.Vector(-amount, 0, 0)
    moves[62] = unreal.Vector(0, 0.18, -0.08)
    moves[6] = unreal.Vector(0, 0.08, 0)
    CREATOR.translate_face_landmarks(character, list(moves), list(moves.values()))
    CREATOR.commit_face_state(character)

    measurements = list(CREATOR.get_body_constraints(character))
    targets = {"Height": 181.0, "Chest": 92.0, "Waist": 75.0}
    for constraint in measurements:
        if str(constraint.name) in targets:
            constraint.is_active = True
            constraint.target_measurement = targets[str(constraint.name)]
    CREATOR.set_body_constraints(character, measurements)
    CREATOR.commit_body_state(character)

    settings = character.get_editor_property("skin_settings")
    skin = settings.skin
    skin.u = 0.39
    skin.v = 0.45
    skin.roughness = 1.04
    settings.skin = skin
    character.preview_material_type = unreal.MetaHumanCharacterSkinPreviewMaterial.EDITABLE
    CREATOR.commit_skin_settings(character, settings)


def color_wardrobe(character):
    CREATOR.assemble_for_preview(character)
    instance = CREATOR.get_preview_collection(character).default_instance
    hair_pigment = {"Hair": 0.64, "Eyebrows": 0.72, "Mustache": 0.66, "Beard": 0.66}
    outfit_colors = {
        "PrimaryColorShirt": (0.21, 0.22, 0.23), "SecondaryColorShirt": (0.13, 0.14, 0.15),
        "PrimaryColorShort": (0.022, 0.025, 0.030), "SecondaryColorShort": (0.031, 0.035, 0.041),
    }
    for selection in instance.get_slot_selections():
        slot = str(selection.slot_name)
        if slot not in hair_pigment and slot != "Outfits":
            continue
        path = unreal.MetaHumanPaletteItemPath(item_key=selection.selected_item)
        for parameter in instance.get_instance_parameters(path):
            name = str(parameter.name)
            if slot == "Outfits" and name in outfit_colors:
                parameter.set_color(unreal.LinearColor(*outfit_colors[name], 1))
            elif name == "Melanin":
                parameter.set_float(hair_pigment[slot])
            elif name == "Redness":
                parameter.set_float(0.29)
    CREATOR.on_edit_preview_collection(character)


def export_geometry(character):
    head = ANATOMY + "/Max_Head"
    body = ANATOMY + "/Max_Body"
    params = unreal.MetaHumanGeometryExportParams()
    params.project_path = ANATOMY
    # Leave existing meshes and any game material assignments intact on reruns.
    params.head_skeletal_mesh = not ASSETS.does_asset_exist(head)
    params.body_skeletal_mesh = not ASSETS.does_asset_exist(body)
    params.full_body_skeletal_mesh = False
    if params.head_skeletal_mesh or params.body_skeletal_mesh:
        unreal.MetaHumanCharacterExportBlueprintLibrary.export_geometry(character, params)
    for path in [head, body]:
        if not ASSETS.does_asset_exist(path) or not ASSETS.save_asset(path):
            raise RuntimeError("Missing geometry export " + path)


def bake_face_map(texture, name, folder, channel):
    # The interactive skin synthesizer stores pixels in GPU mips, without an
    # importable texture source. Read its uncompressed BGRA mip through the RHI.
    scratch = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_intermediate_dir())) / "MaxCreator"
    scratch.mkdir(parents=True, exist_ok=True)
    image = scratch / (name + ".png")
    options = unreal.ImageWriteOptions()
    options.format = unreal.DesiredImageFormat.PNG
    options.overwrite_file = True
    options.set_editor_property("bAsync", False)
    unreal.ImageWriteBlueprintLibrary.export_to_disk(texture, str(image), options)
    if not image.is_file() or image.stat().st_size < 6000:
        raise RuntimeError("Creator produced an empty preview image: " + name)
    task = unreal.AssetImportTask()
    task.filename = str(image)
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    TOOLS.import_asset_tasks([task])
    result = unreal.load_asset(folder + "/" + name)
    result.set_editor_property("srgb", channel == "BaseColor")
    if channel == "Normal":
        result.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif channel == "Cavity":
        result.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    return result


def export_skin_maps(character):
    # Epic's complete material export requires high-resolution texture sources.
    # Creator also provides these local skin maps for offline editing.
    texture_folder = ANATOMY + "/Textures"
    texture_sources = [
        ("Face", character.get_editor_property("synthesized_face_textures"), unreal.FaceTextureType, ""),
        ("Body", character.get_editor_property("body_textures"), unreal.BodyTextureType, "BODY_"),
        ("Chest", character.get_editor_property("body_textures"), unreal.BodyTextureType, "CHEST_"),
    ]
    for region, source, enum, prefix in texture_sources:
        for channel in ["BaseColor", "Normal", "Cavity"]:
            name = "T_Max_" + region + "_" + channel
            if ASSETS.does_asset_exist(texture_folder + "/" + name):
                continue
            texture = source[getattr(enum, prefix + channel.upper())]
            copied = (bake_face_map(texture, name, texture_folder, channel) if region == "Face"
                      else TOOLS.duplicate_asset(name, texture_folder, texture))
            if not copied or not ASSETS.save_loaded_asset(copied, False):
                raise RuntimeError("Could not persist Creator texture " + name)
            unreal.log(f"MAX CREATOR {name} {copied.blueprint_get_size_x()}x{copied.blueprint_get_size_y()}")


def main():
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT], True)
    is_new = not ASSETS.does_asset_exist(CHARACTER)
    character = TOOLS.duplicate_asset("Max", ROOT + "/Creator", unreal.load_asset(PRESET)) if is_new else unreal.load_asset(CHARACTER)
    if not isinstance(character, unreal.MetaHumanCharacter):
        raise RuntimeError("MetaHuman Creator and its Core Data are required.")
    if character.internal_collection.get_outer() != character:
        raise RuntimeError("The editable wardrobe must belong to Max.")
    if is_new:
        character.get_editor_property("assembly_settings").set_editor_property("name_override", "Max")
        select_grooms(character)
    if not CREATOR.try_add_object_to_edit(character):
        raise RuntimeError("Close Max's character editor before running this script.")
    try:
        if is_new:
            sculpt(character)
            color_wardrobe(character)
            if not ASSETS.save_loaded_asset(character, False):
                raise RuntimeError("Could not save Max.")
        export_geometry(character)
        export_skin_maps(character)
    finally:
        CREATOR.remove_object_to_edit(character)
    unreal.log("MAX CREATOR READY " + character.get_path_name())


main()
