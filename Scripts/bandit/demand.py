"""Fit the demand pose to the bandit's face and one-handed silhouette."""
import unreal as u

from bandit.head import ROOT
from hermit.pose import build_aim


def build():
    source_path = "/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS"
    target = build_aim(base=source_path, head_path=ROOT + "/Villa_Head",
                       name="A_BanditDemand", folder=ROOT, upright_weight=.82)
    source = u.load_asset(source_path)
    idle = u.load_asset("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle")
    # Keep the free hand by the belt, leaving the bandoliers and collar visible.
    arm = ("clavicle", "upperarm", "lowerarm", "hand", "thumb", "index", "middle", "ring", "pinky")
    tracks = {bone: ([], [], []) for bone in u.AnimationLibrary.get_animation_track_names(source)
              if str(bone).endswith("_l") and str(bone).startswith(arm)}
    options = u.AnimPoseEvaluationOptions()
    get = u.AnimPoseExtensions.get_bone_pose
    local = u.AnimPoseSpaces.LOCAL
    world = u.AnimPoseSpaces.WORLD
    end = u.AnimationLibrary.get_num_frames(source)
    for frame in range(end + 1):
        rest = u.AnimPoseExtensions.get_anim_pose_at_time(idle, idle.get_play_length() * frame / end, options)
        aim = u.AnimPoseExtensions.get_anim_pose_at_frame(source, frame, options)
        for bone, (positions, rotations, scales) in tracks.items():
            tr = get(rest, bone, local)
            rotation = tr.rotation
            if str(bone) == "clavicle_l":
                parent = get(aim, bone, world).rotation * get(aim, bone, local).rotation.inversed()
                rotation = parent.inversed() * get(rest, bone, world).rotation
            positions.append(tr.translation)
            rotations.append(rotation)
            scales.append(tr.scale3d)
    controller = target.get_editor_property("controller")
    controller.open_bracket("Set the bandit's free arm", False)
    try:
        for bone, keys in tracks.items():
            if not controller.set_bone_track_keys(bone, *keys, False):
                raise RuntimeError("Could not set " + str(bone))
    finally:
        controller.close_bracket(False)
    if not u.EditorAssetLibrary.save_loaded_asset(target, False):
        raise RuntimeError("Could not save the bandit's demand pose")
