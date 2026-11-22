"""Raise the eyes above the firearm while retaining the original hold and breathing."""

import unreal as u
from hermit.appearance import ROOT, duplicate, save


def build_aim():
    base = "/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS"
    source = u.load_asset(base)
    target = duplicate(base, "A_HermitAim")
    bind = u.SkeletonModifier()
    bind.set_skeletal_mesh(u.load_asset(ROOT + "/Hermit_Head"))
    upright = bind.get_bone_transform("head", True).rotation
    positions, rotations, scales = [], [], []
    eval_options = u.AnimPoseEvaluationOptions()
    for frame in range(u.AnimationLibrary.get_num_frames(source) + 1):
        pose = u.AnimPoseExtensions.get_anim_pose_at_frame(source, frame, eval_options)
        local = u.AnimPoseExtensions.get_bone_pose(pose, "head", u.AnimPoseSpaces.LOCAL)
        original = u.AnimPoseExtensions.get_bone_pose(pose, "head", u.AnimPoseSpaces.WORLD).rotation
        parent = original * local.rotation.inversed()
        facing = original.slerp_quat(upright, 0.94)
        positions.append(local.translation)
        rotations.append(parent.inversed() * facing)
        scales.append(local.scale3d)
    ctl = target.get_editor_property("controller")
    if not ctl.set_bone_track_keys("head", positions, rotations, scales, False):
        raise RuntimeError("Could not author the resident head track")
    save(target)
