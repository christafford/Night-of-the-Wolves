# Project instructions

- Use the local Linux source build of UE 5.9 Preview 2 at `~/UnrealEngine-5.9-preview-2`. Check unfamiliar APIs against its source and headers.
- Read `docs/GAME_DESIGN.md`, `docs/MILESTONES.md` and `mynotes/NIGHT_OF_THE_WOLVES_FIRST_LEG.md` for the intended progression. The first leg continues through the creek and the bandit to the escape bend (CP5). A lightning-struck fir blocks the road beyond CP5. Fire from the trunk opens a root-screened cave passage to CP6. The forest transcript provides art direction. Current authoring choices and sources are in `docs/FOREST.md`.
- Work in playable increments. Keep gameplay rules in C++ and expose tuning values where appropriate. Use Unreal Editor operations for binary assets. Record meaningful verification in `docs/PLAYTEST.md`, including the build configuration and whether input, graphics or audio were exercised.
- Write HUD context as something Max can notice. Show controls for available actions and let players infer puzzle solutions and experiment across retries.
- Use the saved startup map `/Game/NightOfTheWolves/Maps/M1_Trail` and its `WolfTrailGameMode`. The template map remains at `/Game/ThirdPerson/Lvl_ThirdPerson`. Keep route geometry, pursuit spline, cottage interactions and creek landing aligned when editing the level.
- The saved assets are ready to play. Editor Python imports and dresses assets. The packaged game uses cooked assets, baked foliage, a PCG graph and native gameplay code.

For wolf behavior and verification, see the [pack design brief](docs/WOLVES.md) and the [implemented controls, tuning and Development test commands](docs/WOLF_PACK.md). The game mode owns the five persistent pawns throughout the leg.

## Build and open the editor

```sh
export UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
PROJECT_DIR="$HOME/Documents/Unreal Projects/Night_of_the_Wolves"
UE_PROJECT="$PROJECT_DIR/Night_of_the_Wolves.uproject"

"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" Night_of_the_WolvesEditor Linux Development \
  -Project="$UE_PROJECT" -WaitMutex
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$UE_PROJECT"
```

Press Play in the startup map. `BP_Max` inherits third-person movement, camera and input from the template. His locomotion Blueprint animates the MetaHuman skeleton, and the game C++ classes drive equipment, sprint, interactions and the cable trolley. See [Max authoring](docs/MAX.md) for the Creator asset and the cooked character assembly.

## Package and launch on Linux

After setting the variables above, build, cook and archive the first leg with the project's established UAT command.

```sh
UE_OUTPUT="$PROJECT_DIR/Saved/MoonlitTrail-Linux"
mkdir -p "$UE_OUTPUT"
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$UE_PROJECT" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -package -archive -archivedirectory="$UE_OUTPUT" \
  -utf8output -unattended

"$UE_OUTPUT/Linux/Night_of_the_Wolves.sh"
```

Set `UE_OUTPUT="$HOME/Builds/NightOfTheWolves-Linux"` to use the original external archive location. Current playtest instructions use `Saved/MoonlitTrail-Linux`.

## Play the river, bandit and storm checkpoints

Run `./Scripts/play_checkpoint.sh river` for the broken bridge, `./Scripts/play_checkpoint.sh bandit` for the far bank or `./Scripts/play_checkpoint.sh storm` for the escape bend. They launch the archived Shipping game with a temporary copy of an earned checkpoint from `Tests/Fixtures`. A regular launch resumes the player’s own save.

For an uninterrupted Development run from the trailhead through the cave, use `-FirstLegTest -FirstLegClean -IntoStorm`. The full first-leg recovery driver also throws the sack uphill, loses it, retries and throws into the approaching pack. To test the robbery in isolation, copy the CP4 fixture into a private save directory as described below and run `-ExecCmds="t.MaxFPS 0,Wolf.TestRobbery"`. It exercises waiting and sprinting on both the road and its shoulder. To exercise the cave in isolation, copy the bend fixture to `Saved/SaveGames/MoonlitTrail-v1.sav` inside a temporary user directory. Use `-SaveToUserDir -UserDir=<absolute temporary directory>` with `-ExecCmds="t.MaxFPS 0,Wolf.TestStormfall"`. Repeat against the resulting save with `-StormResume` to check cold CP6. Add `-StormPhotos` for rendered frames. `-StormRetry` retries during the pack’s reaction shot and `-StormLinger` waits in the clearing for over a game minute with an unlit brand.

## Verify a gameplay change

The editor can run the physical route headlessly. The clean run begins with empty hands and uses normal movement, interactions and equipment rules. Check the pass message and process exit status.

```sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$UE_PROJECT" \
  /Game/NightOfTheWolves/Maps/M1_Trail -game -unattended -nop4 -nullrhi -nosound \
  -UseFixedTimeStep -FPS=60 -ExecCmds='t.MaxFPS 0' \
  -FirstLegTest -FirstLegClean -abslog=/tmp/wolves-clean.log
rg 'FIRST LEG CLEAN PASSED|TEST FAILED' /tmp/wolves-clean.log
```

For the full recovery route, omit `-FirstLegClean`. Add `-DoorwayTest` to exercise empty-handed and extinguished-brand departures, both CP2 retries, and then a successful exit with the flame. The earlier `-OmitFirebrand` flag also exercises this revised rule. Use `-CottageRecoveryTest` in place of the first-leg flags to check the unprepared knock, inventory restoration and door breach. These drivers are excluded from Shipping and isolate test progress from the live checkpoint. For an ordinary unsaved QA session, use `-NoTrailSave`. See the playtest record for the fresh-process disk check and manual cases.

On a fresh trail, run the editor game with `-NoTrailSave` and `-ExecCmds="t.MaxFPS 0,Wolf.TestOpening"` using the fixed-step flags above. Check for `OPENING TEST PASSED` and exit code 0. The opening driver watches the title dissolve, camera travel, return to Max, full idle lead and first retry. The route and cottage regression drivers start directly in gameplay.

For art changes, inspect real rendered views from the player camera. A renderer-backed development run with `-FirstLegTest -FirstLegClean -FirstLegPhotos` captures encounter views under `Saved/FirstLegReview`. Verify packaged startup and input separately.

Use `-CottageRecoveryTest -CottagePhotos` to capture the armed run, the aim on the intact boards, and the fall into the open hollow. Use `-FirstLegTest -DoorwayTest -EncounterPhotos -WolfCapturePhotos` for the waiting pack, flame retreat and wolf pounce under `Saved/EncounterReview`. Add `-HermitPortrait` to the cottage photo run for a closer look at the beard and eyes in the porch light. On a fresh trail, the development console command `Wolf.TestCapture` checks impact, death-card order and both retry paths.

## Earlier prototypes

The initial template and milestone packages established the Linux build process. Historical archives include `M1-Linux`, `M2-Linux`, `M3-Linux`, `M4-Linux`, `Winding-Linux` and `Forest-Linux` under the project’s `Saved` directory. Their acceptance results and former controls are retained in the historical portion of `docs/PLAYTEST.md`.

`Scripts/create_m1_map.py` records the original editable blockout and refuses to overwrite the map. The M2–M4 upgrade scripts, winding-approach script and early forest scripts record later iterations. For current procedural foliage and sky work, use the workflow in `docs/FOREST.md`.
