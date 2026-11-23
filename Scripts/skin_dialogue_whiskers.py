"""Bind the original authored hair cards to each character's moving facial skin in Unreal Editor."""
from collections import defaultdict
from pathlib import Path
import sys

import unreal as u

sys.path.insert(0, str(Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())) / "Scripts"))
from create_max_outfit import save_mesh


def connected_cards(mesh, count):
    # Each disconnected ribbon is one lock. All of its vertices must follow the
    # same skin patch; proximity transfer at the tip would bind long beards to the neck.
    groups = list(range(count))

    def group(i):
        while i != groups[i]:
            groups[i] = groups[groups[i]]
            i = groups[i]
        return i

    triangles = u.GeometryScript_List.convert_triangle_list_to_array(mesh.get_all_triangle_indices(False)[-2])
    for t in triangles:
        a, b, c = group(t.x), group(t.y), group(t.z)
        groups[b] = groups[c] = a
    cards = defaultdict(list)
    for i in range(count):
        cards[group(i)].append(i)
    return cards.values()


def skin(folder, face_name, groom_name):
    face_asset = u.load_asset(f"{folder}/{face_name}")
    original = u.load_asset(f"{folder}/SM_{groom_name}")
    face, hair = u.DynamicMesh(), u.DynamicMesh()
    options = u.GeometryScriptCopyMeshFromAssetOptions()
    lod = u.GeometryScriptMeshReadLOD()
    for copy, asset, dynamic in (
        (u.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh, face_asset, face),
        (u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh, original, hair),
    ):
        if copy(asset, dynamic, options, lod)[-1] != u.GeometryScriptOutcomePins.SUCCESS:
            raise RuntimeError(f"Could not read {asset.get_name()}")
    rig = u.SkeletonModifier()
    rig.set_skeletal_mesh(face_asset)
    hair.transform_mesh(rig.get_bone_transform("head", True))
    face.transfer_bone_weights_from_mesh(hair)
    pos = u.GeometryScript_List.convert_vector_list_to_array(hair.get_all_vertex_positions(False)[-2])
    cards = connected_cards(hair, len(pos))
    if len(cards) < 100:
        raise RuntimeError(f"Expected separate hair cards in {original.get_name()}")
    for card in cards:
        # Keep the dense sculpted moustache foundation's continuous weights.
        # The short, fine strands and beard locks instead inherit the root weights.
        if len(card) > 150:
            continue
        root = max(card, key=lambda i: pos[i].z)
        weights, valid = hair.get_vertex_bone_weights(root)[-2:]
        if not valid or not weights:
            raise RuntimeError("Missing hair root weights")
        for i in card:
            hair.set_vertex_bone_weights(i, weights)
    material = original.get_material(0)
    material.set_editor_property("used_with_skeletal_mesh", True)
    u.EditorAssetLibrary.save_loaded_asset(material)
    name = "SK_" + groom_name
    if not u.EditorAssetLibrary.does_asset_exist(f"{folder}/{name}"):
        save_mesh(hair, name, face_asset.skeleton, {"Hair": material}, folder)
    # UE 5.9's Geometry Script asset update also enables the render vertex-color
    # buffer. The create operation alone only preserves colors in the source model.
    asset = save_mesh(hair, name, face_asset.skeleton, {"Hair": material}, folder)
    u.log(f"DIALOGUE WHISKERS {asset.get_name()} {len(cards)} locks and surfaces")


skin("/Game/NightOfTheWolves/FirstLeg/Hermit", "Hermit_Head", "HermitWhiskers")
skin("/Game/NightOfTheWolves/Bandit", "Villa_Head", "VillaHair")
