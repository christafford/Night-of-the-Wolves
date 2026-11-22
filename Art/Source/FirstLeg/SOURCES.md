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

## Props, voice and ambience

The shovel, rope coil, coin pouch, firebrand and early static wolf come from [the prop generator](../../../Scripts/make_first_leg_props.py). It also synthesizes the hearth, door impact, growl and gunshot. Run it with Python 3 to regenerate the GLB and WAV sources, then use the [Unreal import script](../../../Scripts/import_first_leg_props.py).

The original dialogue uses a temporary synthetic performance. The forest and presentation credits cover the photographic woodland assets, wind, howls and earlier materials. [Voice credits](Voices/SOURCES.md) · [Forest credits](../../../docs/FOREST.md) · [Presentation credits](../Presentation/SOURCES.md)
