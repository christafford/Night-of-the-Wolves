# Max

Open `/Game/NightOfTheWolves/Max/Creator/Max` in MetaHuman Creator to edit Max. The local UE 5.9 Preview 2 plugin and its Core Data supply the Mateo starting point. Max has a narrower lower jaw and nostrils, a slightly projected chin, and a warm, fair complexion. His Creator body targets 181 cm in height, a 92 cm chest and a 75 cm waist. His selected grooms are Short Side Swept Fringe hair, Medium Natural eyebrows and short beard and mustache stubble. Brown hair uses 0.64 melanin and 0.29 redness. The skin picker uses U 0.39 and V 0.45.

The playable pawn is `/Game/NightOfTheWolves/Max/BP_Max`. The `Anatomy` folder holds Max's Creator-exported skeletal head and body, local 1K facial skin images and 2K body skin images. The body material uses Max’s saved Creator bias and gain to reconstruct hand color from its neutral body texture. The `Wardrobe` folder holds the runtime clothing and exposed body. The indigo jacket, ochre yoke, gray shirt, dark jeans and oxblood sneakers are project-authored clothing based on the silhouette and colors in the supplied runner photograph. The photograph stays outside the game assets. [Visual reference](https://as2.ftcdn.net/jpg/02/06/28/07/1000_F_206280746_q9eKQ3nva9cuhLm3Um5fA8zkFZPI0drg.jpg)

The playable version uses cooked meshes, textures and animation. On Vulkan with strand rendering enabled, two MetaHuman groom bindings anchor the fringe and eyebrows to Max’s animated head. When strand rendering is unavailable, the game uses fitted hair cards and baked brows. Max currently has a neutral facial pose. Epic’s cloud autorig can add expression-ready facial DNA, and Creator’s Download Texture Sources action can supply higher-resolution skin.

## Local authoring

Run these commands in the project root with Max's asset editor closed. The Vulkan editor supplies the face texture readback. The full editor also supplies the Content Browser used by Creator's geometry export. The trail uses the saved assets directly.

```sh
export UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
UE_PROJECT="$PWD/Night_of_the_Wolves.uproject"

for script in Scripts/max/create_max.py Scripts/create_max_outfit.py \
              Scripts/max/finish_appearance.py Scripts/max/bind_grooms.py Scripts/max/assemble_player.py
do
    "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$UE_PROJECT" /Engine/Maps/Entry \
      -EnablePlugins=MetaHumanCharacter,GeometryScripting \
      "-ExecutePythonScript=$PWD/$script" -RenderOffScreen -unattended -nop4 -nosound || break
done
```

`create_max.py` sculpts a fresh character and generates missing exports. Reruns preserve existing face edits, geometry and skin maps. For a deliberate resculpt, edit Max in Creator, re-export the changed head and body to `Anatomy`, and refit the outfit and hair with the remaining scripts. The editor build, Shipping archive and launch commands are in [AGENTS.md](../AGENTS.md).

## Visual review

To inspect Max under a soft key light on the trail, launch this development-only review from the project root. It writes front, back and portrait views to `Saved/MaxReview` and exits. Ordinary gameplay uses the forest and fireplace lighting.

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$UE_PROJECT" \
  /Game/NightOfTheWolves/Maps/M1_Trail -game -NoTrailSave -nosound -nop4 \
  -windowed -ResX=1280 -ResY=800 -UseFixedTimeStep -FPS=40 -NoAsyncShaderCompile \
  -ExecCmds='t.MaxFPS 0,Max.Portrait'
```
