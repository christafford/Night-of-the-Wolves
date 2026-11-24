# Hermit cabin carpentry

The log shell, exposed end-grain rings, staggered roof shakes, ledged-and-braced door and its ironwork were built for this game. The hewn logs use the Rough Wood scan. The floors, shakes and door use Brown Planks 09. Both sets supply 2K diffuse and DirectX normal maps and a 1K roughness map. [Rough Wood](https://polyhaven.com/a/rough_wood) · [Brown Planks 09](https://polyhaven.com/a/brown_planks_09) · [Poly Haven CC0 license](https://polyhaven.com/license)

Run the [download script](../../../../Scripts/cabin/fetch_carpentry.py) to refresh the six source maps. With the project’s engine and project variables set, the following command rebuilds the two editable mesh assets and installs them in the playable map.

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript -script="$PROJECT_DIR/Scripts/cabin/build_log_cabin.py" \
  -nullrhi -nosound -unattended -nop4
```

The existing ceiling and wall pieces provide collision. The visible door leaf is a movable child of the original hinged blocking panel. The timber frame fits the existing door and window openings, and the plank floor starts inside the pit edge.

The [cottage supplies script](../../../../Scripts/cabin/finish_cottage_supplies.py) models the three-strand rope, whipping at the coil's head, puckered linen purse and knotted drawcord. It preserves the shared pickup and equipment pivots and adds soft hearth bounce to the worktable and peg in the saved map. Run it after the carpentry command when rebuilding the room.

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript -script="$PROJECT_DIR/Scripts/cabin/finish_cottage_supplies.py" \
  -nullrhi -nosound -unattended -nop4
```
