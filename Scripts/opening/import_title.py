"""Import the film title into its hard-referenced HUD slot using Unreal Editor."""
from pathlib import Path
import unreal as u

source = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())) / "Art/Source/Presentation/Opening/T_OpeningTitle.png"
destination = "/Game/NightOfTheWolves/Presentation"
task = u.AssetImportTask()
task.set_editor_properties(dict(filename=str(source), destination_path=destination,
                                automated=True, replace_existing=True, save=True))
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = u.EditorAssetLibrary.load_asset(destination + "/T_OpeningTitle")
if not isinstance(texture, u.Texture2D):
    raise RuntimeError("Opening title failed to import")
# BC7 preserves the engraved strokes and shadow gradations at one byte per
# pixel. The non-streaming UI plate is resident at first draw.
texture.set_editor_properties(dict(lod_group=u.TextureGroup.TEXTUREGROUP_UI,
                                   compression_settings=u.TextureCompressionSettings.TC_BC7,
                                   compression_no_alpha=True, srgb=True, never_stream=True,
                                   mip_gen_settings=u.TextureMipGenSettings.TMGS_NO_MIPMAPS,
                                   address_x=u.TextureAddress.TA_CLAMP, address_y=u.TextureAddress.TA_CLAMP))
if not u.EditorAssetLibrary.save_loaded_asset(texture, False):
    raise RuntimeError("Opening title failed to save")
u.log("OPENING TITLE SAVED " + texture.get_path_name() + " " +
      str((texture.blueprint_get_size_x(), texture.blueprint_get_size_y())))
