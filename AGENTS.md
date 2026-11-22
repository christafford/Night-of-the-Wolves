# Project instructions

- This is a Linux Unreal Engine C++ project. The source-built engine is `~/UnrealEngine-5.9-preview-2` (UE 5.9 Preview 2); the editor is `Engine/Binaries/Linux/UnrealEditor` beneath it. Consult the local engine source and headers when checking uncertain APIs.
- Read `docs/GAME_DESIGN.md` for the intended experience and unresolved decisions, and `docs/MILESTONES.md` for the proposed sequence. Do not treat suggestions from the earlier conversation as confirmed requirements.
- Work in small playable increments. Prefer C++ for gameplay rules and editor-configurable properties, with Unreal assets/Blueprints for presentation where appropriate. Use Unreal tools or editor APIs for binary assets, and document any manual editor steps.
- The startup map is `/Game/NightOfTheWolves/Maps/M1_Trail`; the template map remains at `/Game/ThirdPerson/Lvl_ThirdPerson`. `Scripts/create_m1_map.py` documents creation of the initial blockout and refuses to overwrite an existing map; `Scripts/upgrade_m2_map.py`, `Scripts/upgrade_m3_map.py`, `Scripts/upgrade_m4_presentation.py`, and `Scripts/refine_m4_forest.py` record later editor changes. These are editor-only, not runtime gameplay.
- Preserve the working Linux build process below. Identify placeholders, and report compilation, launch, gameplay checks, and visual review separately according to what was actually verified. Record meaningful playtest results in `docs/PLAYTEST.md`.

## Known packaging command

The user reports this command successfully built, cooked, packaged, and archived the default project, and that the resulting Linux executable ran locally. It has not yet been independently rerun in this session.

```sh
export UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
UE_PROJECT="$HOME/Documents/Unreal Projects/Night_of_the_Wolves/Night_of_the_Wolves.uproject"
UE_OUTPUT="$HOME/Builds/NightOfTheWolves-Linux"
mkdir -p "$UE_OUTPUT"
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$UE_PROJECT" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -package -archive -archivedirectory="$UE_OUTPUT" \
  -utf8output -unattended
```

The archive directory is outside the project workspace; access may require permission in the active environment.

## Editor build and launch

```sh
export UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
UE_PROJECT="$HOME/Documents/Unreal Projects/Night_of_the_Wolves/Night_of_the_Wolves.uproject"
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" Night_of_the_WolvesEditor Linux Development -Project="$UE_PROJECT" -WaitMutex
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$UE_PROJECT"
```

In the editor, open the M1 startup map and press Play. The saved map already assigns `WolfTrailGameMode`; no manual setup is intended. The third-person template Blueprint supplies movement, camera, and input. UE's source build may write build logs and caches outside this workspace.

The M1 verification package was archived to `Saved/M1-Linux/Linux`. Launch it with `"$HOME/Documents/Unreal Projects/Night_of_the_Wolves/Saved/M1-Linux/Linux/Night_of_the_Wolves.sh"`. It is a local build artifact, not a replacement for the user's usual archive directory.

The newer M2 blockout package is in `Saved/M2-Linux/Linux`, with the same launcher name. It includes the floor correction and cabin draft; see `docs/PLAYTEST.md` for outstanding checks.

The M3 package is built to `Saved/M3-Linux/Linux`. The non-Shipping `-M3SelfTest` option drives the physical prototype route headlessly; `-M3ShovelTimeTest` checks the pickup bonus and capture timing. They invoke actions programmatically and do not replace a keyboard or visual playtest.

The M4 presentation prototype is packaged under `Saved/M4-Linux/Linux`; see `docs/PLAYTEST.md` for the review status and `Art/Source/Presentation/SOURCES.md` for asset sources and licensing.
