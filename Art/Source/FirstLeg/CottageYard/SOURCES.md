# Cabin yard storage

The field shovel uses Blemonade’s scan for Poly Haven of a worn, T-handled spade. The model is 123 cm long and its origin sits in the wooden shaft where Max grips it. The yard and equipment use the same mesh. James Ray Cock’s cask retains its lid, bung, curved staves and iron hoops. The scans use 1K base color, DirectX normal and packed occlusion, roughness and metalness maps. [Rusted Spade 01](https://polyhaven.com/a/rusted_spade_01) · [Wine Barrel 01](https://polyhaven.com/a/wine_barrel_01) · [Poly Haven CC0 license](https://polyhaven.com/license)

The rectangular bale is original geometry. Two lengths of twisted twine compress the flanks, with short broken stalks at the cut ends and bent stalks along the surface. Its underlying material comes from TextureCan. [Dry Straw Grass / Hay, Ground 0018](https://www.texturecan.com/details/190/) · [TextureCan CC0 license](https://www.texturecan.com/terms/)

The source meshes and their textures are checked in beside this file. To regenerate them, run the first command with Python 3 and Pillow. The import uses the full editor so it can save the cask and bale’s simple collision. Set the engine and project variables as in [AGENTS.md](../../../../AGENTS.md).

```sh
python3 Scripts/cottage/prepare_yard.py
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -ExecutePythonScript="$PWD/Scripts/cottage/import_yard.py" -nullrhi -nosound -unattended -nop4
```
