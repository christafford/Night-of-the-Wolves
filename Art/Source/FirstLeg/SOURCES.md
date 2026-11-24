# First-leg art and sound

## Timber wolves

The live pack takes its silhouette and markings from the project's [wolf death illustration](../Presentation/wolf_attack.png). The opaque coat carries the broad saddle, cheek and muzzle markings. The exposed nose pad uses its own [grain normal](Wolves/TimberNoseLeather.png). Overlapping, skinned guard hairs break up the outline around the cheeks, neck and tail. The live trot, swim and pounce use the rig in [SK_TimberWolf.glb](Wolves/SK_TimberWolf.glb). A soft, cool reflection lights the pack on its own Unreal lighting channel. The pack component exposes its intensity as `MoonBounceCandelas`.

The [wolf authoring script](../../../Scripts/wolves/author_timber_wolf.py) builds the mesh, weights and masked guard-hair atlas. It crops the death illustration into a grayscale pigment map and projects that onto the modeled forehead and muzzle. It requires Python 3 and ImageMagick 7. The [Unreal Editor import script](../../../Scripts/wolves/import_timber_wolf.py) binds the mesh and its five materials. With the project variables from [AGENTS.md](../../../AGENTS.md), run it from the project root with a rendering commandlet.

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" -run=pythonscript \
  "-script=$PWD/Scripts/wolves/import_timber_wolf.py" -AllowCommandletRendering -RenderOffScreen -unattended -nop4 -nosound
```

The [undercoat albedo](Wolves/TimberUndercoat.png) was generated with Codex's built-in image generation, then resized to 1024 × 1024. The [tangent-space normal](Wolves/TimberUndercoatNormal.png) gives individual strands fine relief under changing light. Regenerate it with `python Scripts/wolves/bake_undercoat_normal.py` using Pillow and NumPy.

<details>
<summary>Undercoat generation prompt</summary>

Square photorealistic seamless material swatch for a 3D timber wolf winter coat. Straight overhead macro wildlife photograph of dense layered grey wolf neck fur only, covering the entire frame edge to edge. A twelve-centimeter patch of overlapping tapered strands swept from top to bottom with small natural variations, a dense warm-grey woolly undercoat and long banded salt-and-pepper guard hairs. Charcoal roots and silver cream tips form fine and medium-scale texture. Flat diffuse shadowless neutral studio light suitable as a base color texture, even focus throughout. Uniform scale and grain, visually tileable in both directions. No animal face or body outline, background, diagonally rotated pattern, text or border.

</details>

## Cottage hermit

The hermit's appearance follows the supplied double portrait of Popcorn Sutton. The [hermit authoring script](../../../Scripts/build_cottage_hermit.py) starts from the local MetaHuman Creator Walter preset. The [fitted workwear](../../../Scripts/hermit/workwear.py) follows his moving body. The [hat and sculpt](../../../Scripts/hermit/appearance.py) and the [beard and mustache strand atlas](../../../Scripts/hermit/strand_atlas.py) belong to the project. The head material uses Walter’s age, pore and cavity maps. He runs in the local rifle jog, then holds a head-up pose on the boards. [Portrait reference](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcRrB-HJwsn_XSc7109zPj06oiAdS6yUpUwMdMwcSJWbeb2ATdlP7aRnLu8&s=10)

## Roadside bandit

The supplied black-and-white portrait of Pancho Villa guided the character’s face, moustache, hat and crossed bandoliers. The game uses an original MetaHuman sculpt with skin maps baked in the local editor. The shirt, leather and hat colors are art choices for the moonlit clearing. [Portrait reference](https://hips.hearstapps.com/hmg-prod/images/general-villa-news-photo-1686667587.jpg)

The editable character is `/Game/NightOfTheWolves/Bandit/Creator/Villa`. In the local Unreal Editor, run the head authoring script first to export the sculpt, bake the skin under a renderer and fit the hat and hair. Then run the wardrobe script. It fits the shirt collar to that exported neck and skins the shirt, bandoliers, trousers and boots to the animated body. Both scripts use the project's MetaHuman Character and Geometry Scripting plugins. [Head and hat authoring](../../../Scripts/build_bandit_head.py) · [Costume authoring](../../../Scripts/bandit/campaign_dress.py)

## Props, voice and ambience

The abandoned logging cableway has two original timber stations, a six-spoke iron sheave, an oxide-red hanger and a three-strand hemp sling. The stations reuse the [cabin wood scans](Carpentry/SOURCES.md). Rebuild the five saved meshes and three hardware finishes with the [Unreal Editor authoring script](../../../Scripts/crossing/build_skyline.py).

The wheel roll, shackle clink and end-stop knock are original synthesized sounds. Their editor script saves the three sound waves under `/Game/NightOfTheWolves/Crossing/Foley`. [Foley authoring](../../../Scripts/crossing/build_cableway_foley.py)

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" -run=pythonscript \
  "-script=$PWD/Scripts/crossing/build_skyline.py" -nullrhi -nosound -unattended -nop4
```

The shovel, oak cask and bound hay beside the cottage use the [yard models and PBR sources](CottageYard/SOURCES.md).

The shovel, rope coil, coin pouch, firebrand and early static wolf come from [the prop generator](../../../Scripts/make_first_leg_props.py). It also synthesizes the hearth, door impact, growl and gunshot. Run it with Python 3 to regenerate the GLB and WAV sources, then use the [Unreal import script](../../../Scripts/import_first_leg_props.py).

The hermit and bandit have original AI-generated performances of the game's six dialogue lines. Their dry takes, performance directions and location-specific mixes are documented with the audio sources. [Voice credits](Voices/SOURCES.md) · [Forest credits](../../../docs/FOREST.md) · [Presentation credits](../Presentation/SOURCES.md)

The hermit’s fall uses the Wilhelm scream from Craig Smith’s CC0 preservation of the USC Sunset Editorial tapes. The source reel and exact cut are recorded with the WAV. [Fall sound source](Hermit/SOURCES.md)

After rebuilding either speaking head or its hair cards, run the two editor scripts below. The first fits each beard or moustache lock to the weighted facial surface. The second assigns the MetaHuman enamel, gum and tongue atlas to both speaking heads under the encounter lighting.

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript -script="$PWD/Scripts/skin_dialogue_whiskers.py" -nullrhi -nosound -unattended -nop4
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript -script="$PWD/Scripts/finish_dialogue_mouths.py" -AllowCommandletRendering -nosound -unattended -nop4
```
