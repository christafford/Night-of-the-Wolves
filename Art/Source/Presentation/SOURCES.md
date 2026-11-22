# Prototype presentation assets

- `wolf_attack.png`: generated with OpenAI's built-in image generation tool for this project. Prompt: “2D foreground overlay for a third-person nighttime survival game's brief wolf attack. A realistic terrifying grey wolf lunging directly toward the camera, head and forequarters prominent, snarling and baring teeth, vivid glowing red eyes; dynamic but clearly readable anatomy. Moody cold moonlit highlights on dark fur. Isolated full subject on a genuinely transparent background, clean cutout, centered with breathing room around the silhouette. No text, logos, border, gore, or other animals.” Generated before the user supplied the visual reference; provisional.
- `wolf_howls.wav`: converted to mono 44.1 kHz WAV from [Wolf howls](https://commons.wikimedia.org/wiki/File:Wolf_howls.ogg), U.S. Fish and Wildlife Service, public domain as a U.S. federal government work.
- `forest_wind.wav`: converted to mono 44.1 kHz WAV from [Vent](https://commons.wikimedia.org/wiki/File:Joseph_SARDIN_-_Vent.ogg), Joseph Sardin, CC0 1.0.
- `forest_ground_04.jpg`, `mud_forest.jpg`, `mossy_stone_wall.jpg`, and `bark_willow.jpg`: 1K diffuse textures from Poly Haven, CC0: [Forest Ground 04](https://polyhaven.com/a/forest_ground_04), [Mud Forest](https://polyhaven.com/a/mud_forest), [Mossy Stone Wall](https://polyhaven.com/a/mossy_stone_wall), and [Bark Willow](https://polyhaven.com/a/bark_willow).
- `mossy_cobblestone.jpg`: 1K diffuse texture from [Mossy Cobblestone](https://polyhaven.com/a/mossy_cobblestone), Poly Haven, CC0.
- `grey_stone_path.jpg`: 1K diffuse texture from [Grey Stone Path](https://polyhaven.com/a/grey_stone_path), Poly Haven, CC0; used on the revised trail.

These are a first prototype presentation pass. The reference image is a style reference and is not included as a game asset.
- `../ForestTrees/dead_tree.fbx` and its embedded textures: [Dead Tree](https://grab3d.com/models/nature/dead-tree/) by Grab3D, offered under CC0. Used as a provisional low-poly PCG tree.
- `../ForestTrees/island_tree_02.fbx` and its 1K textures: [Island Tree 02](https://polyhaven.com/a/island_tree_02), Poly Haven, CC0. Imported for evaluation only; its materials need setup and it is not used by the PCG graph.

## Opening pursuit audio

`WolfPOVRunningBreath.wav` is original synthesized canine breathing. The 48 kHz mono loop contains 53 different exhalations and intakes over 20 seconds. A broad, rough exhale and shorter nasal intake follow each of the opening camera’s 2.65 strides per second. The moving camera plays the cooked wave at the listener during the run and fades it into the forest mix at the handoff. [Python 3 synthesis](../../../Scripts/opening/render_running_breath.py) · [Unreal Editor import](../../../Scripts/opening/import_running_breath.py)

## Opening title

The opening forest plate is an original image generated for *Night of the Wolves* using Codex image generation. The art brief called for a silver-gelatin nocturne among old-growth Douglas firs and cedars, a small full moon glimpsed through the right canopy, fog across a damp trail and graphite shadows framing the lettering. [Source plate](Opening/MoonlitPlate.png)

The title capitals were drawn for the game. The italic “of the” and the production credit use Noto Serif Display Italic and Regular 2.009, copyright 2022 The Noto Project Authors, under SIL Open Font License 1.1. The packaged game displays the finished raster card. [Editable lettering](Opening/OpeningLettering.svg) · [Noto Serif Display](https://github.com/notofonts/latin-greek-cyrillic) · [Font license](https://openfontlicense.org/open-font-license-official-text/)

With Noto Serif Display, librsvg and ImageMagick installed, run the following from the project root. The editor saves the non-streaming BC7 title texture at `/Game/NightOfTheWolves/Presentation/T_OpeningTitle`.

```sh
python3 Scripts/opening/compose_title.py
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript "-script=$PWD/Scripts/opening/import_title.py" \
  -nullrhi -unattended -nop4 -nosound
```
