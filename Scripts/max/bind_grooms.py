"""Transfer the Creator fringe and brow roots from Epic's original scalp meshes to Max."""

import unreal as u


ROOT = "/Game/NightOfTheWolves/Max/Anatomy"
GROOMS = "/MetaHumanCharacter/Optional/Grooms"


def bind(folder, style, name, target):
    original = u.load_asset(f"{GROOMS}/Bindings/{folder}/{style}_Binding")
    groom = original.get_editor_property("groom")
    source = original.get_editor_property("target_skeletal_mesh")
    path = f"{ROOT}/{name}"
    if u.EditorAssetLibrary.does_asset_exist(path):
        binding = u.load_asset(path)
    else:
        binding = u.GroomLibrary.create_new_groom_binding_asset_with_path(
            path, groom, target, original.get_editor_property("num_interpolation_points"),
            source, original.get_editor_property("matching_section")
        )
    if not binding or binding.get_editor_property("target_skeletal_mesh") != target:
        raise RuntimeError(f"Could not bind {style} to {target.get_name()}")
    u.log(f"MAX GROOM bound {style} from {source.get_name()} at {binding.get_path_name()}")


head = u.load_asset(ROOT + "/Max_Head")
bind("Hair", "Hair_S_SideSweptFringe", "GB_MaxFringe", head)
bind("Eyebrows", "Eyebrows_M_Natural", "GB_MaxBrows", head)
