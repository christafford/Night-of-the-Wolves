"""Verify saved forest material inputs and scene elements without rendering."""
import unreal

root = "/Game/NightOfTheWolves/Presentation/"
for name in ("M_MossyPath", "M_DarkFir"):
    material = unreal.load_asset(root + name)
    if not material:
        raise RuntimeError("Missing material " + name)
    node = unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    if not node:
        raise RuntimeError("Missing base-color input for " + name)
    if name == "M_MossyPath":
        texture = node.get_editor_property("texture")
        if not texture or texture.get_name() != "grey_stone_path":
            raise RuntimeError("Path material is not linked to the irregular stone texture")
    unreal.log("M6 material input verified: " + name + " -> " + node.get_class().get_name())

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level("/Game/NightOfTheWolves/Maps/M1_Trail"):
    raise RuntimeError("Cannot load map")
all_actors = actors.get_all_level_actors()
labels = {a.get_actor_label() for a in all_actors}
for required in ("M5 pursuit route", "M6 cabin spur", "Cabin front right", "M3PitCover"):
    if required == "M3PitCover":
        if not any(unreal.Name(required) in a.get_editor_property("tags") for a in all_actors):
            raise RuntimeError("Missing pit cover tag")
    elif required not in labels:
        raise RuntimeError("Missing " + required)
unreal.log("M6 forest material inputs and encounter assets verified")
