"""Create the playable Max Blueprint from the authored Creator meshes and trail outfit."""

import unreal


ROOT = "/Game/NightOfTheWolves/Max"
PLAYER = ROOT + "/BP_Max"
BASE = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
ASSETS = unreal.EditorAssetLibrary


def require_asset(path, asset_type):
    asset = ASSETS.load_asset(path)
    if not isinstance(asset, asset_type):
        raise RuntimeError(f"Expected {asset_type.__name__} at {path}")
    return asset


def max_locomotion():
    source = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
    path = ROOT + "/ABP_Max"
    editor = unreal.BlueprintEditorLibrary
    blueprint = ASSETS.load_asset(path) if ASSETS.does_asset_exist(path) else ASSETS.duplicate_asset(source, path)
    if not blueprint:
        raise RuntimeError("Could not create Max's locomotion animation Blueprint.")
    graph = editor.find_graph(blueprint, "AnimGraph")
    rigs = [node for node in unreal.ObjectIterator(unreal.AnimGraphNode_ControlRig) if node.get_outer() == graph]
    if len(rigs) != 1:
        raise RuntimeError(f"Expected the template foot placement node in {path}")
    # The mannequin foot IK graph pins the MetaHuman feet at their rest position.
    # Preserve the template state machine, blends and transitions, including jumps.
    rig = rigs[0]
    source_pin = editor.find_input_pin(rig, "Source")
    pose_pin = editor.find_output_pin(rig, "Pose")
    if not source_pin.is_valid() or not pose_pin.is_valid():
        raise RuntimeError("Could not locate the template foot rig pose pins.")
    upstream = source_pin.list_connected_pins()
    destinations = pose_pin.list_connected_pins()
    if destinations:
        if len(upstream) != 1:
            raise RuntimeError("Foot IK has no unique input pose.")
        for destination in destinations:
            if not upstream[0].try_create_connection(destination):
                raise RuntimeError("Could not connect Max's locomotion to the output pose.")
    if not editor.compile_blueprint(blueprint) or not ASSETS.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {path}")
    return blueprint.generated_class()


def assemble():
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
        [ROOT, "/MetaHumanCharacter/Face"]
    )
    parts = {
        "max_body": require_asset(ROOT + "/Wardrobe/SK_MaxExposedBody", unreal.SkeletalMesh),
        "max_head": require_asset(ROOT + "/Anatomy/Max_Head", unreal.SkeletalMesh),
        "max_outfit": require_asset(ROOT + "/Wardrobe/SK_MaxTrailOutfit", unreal.SkeletalMesh),
        "max_hair": require_asset(ROOT + "/Anatomy/SM_MaxHair", unreal.StaticMesh),
        "max_fringe_binding": require_asset(ROOT + "/Anatomy/GB_MaxFringe", unreal.GroomBindingAsset),
        "max_brow_binding": require_asset(ROOT + "/Anatomy/GB_MaxBrows", unreal.GroomBindingAsset),
        "max_locomotion": max_locomotion(),
        "max_face_animation": ASSETS.load_blueprint_class("/MetaHumanCharacter/Face/ABP_Face"),
    }
    if not parts["max_face_animation"]:
        raise RuntimeError("Could not load the Creator face Copy Pose animation Blueprint.")

    if ASSETS.does_asset_exist(PLAYER):
        blueprint = require_asset(PLAYER, unreal.Blueprint)
    else:
        blueprint = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(
            PLAYER, ASSETS.load_blueprint_class(BASE)
        )
        if not blueprint:
            raise RuntimeError(f"Could not create {PLAYER}")

    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.modify()
    for name, value in parts.items():
        defaults.set_editor_property(name, value)
    # Rebuild the generated class's defaults list so freshly spawned pawns
    # receive all the mesh overrides even in this editor session.
    if not unreal.BlueprintEditorLibrary.compile_blueprint(blueprint):
        raise RuntimeError(f"Could not compile {PLAYER}")
    if not ASSETS.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {PLAYER}")
    unreal.log(f"Saved playable Max at {PLAYER}")


assemble()
