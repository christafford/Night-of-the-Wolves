# Playtest record

## Existing baseline

- The user reports that the default C++ project was built and packaged for Linux Shipping with the command in `AGENTS.md`, and the resulting executable ran locally.
- The opening remains a blockout; the M3 cabin solution is implemented provisionally and awaits interactive playtest.

## M1 interactive playtest

The user reports the packaged M1 build showed “You died” after about five seconds, restarted with the correct resets, reached the trail-end message, and killed Max as expected when running into a tree. Max also fell into the void after reaching the trail end. The user later reported that the M2 ground correction worked as expected.

Headless editor-runtime check on UE 5.9 Preview 2: the M1 map loaded with `WolfTrailGameMode`. With `t.MaxFPS 60`, logs showed idle capture approximately five seconds after initialization, followed by a reload approximately two seconds later; this repeated on the next attempt. Null rendering cannot verify the overlay, camera, movement, appearance, or audio. A previous run exposed pawn initialization timing, which was corrected before this check.

The earlier Linux Shipping package built and archived successfully under `Saved/M1-Linux/Linux`. Its staged launcher was started with `-nullrhi -nosound -unattended` and remained running until a 12-second timeout.

```sh
timeout 30s "$HOME/UnrealEngine-5.9-preview-2/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  "$HOME/Documents/Unreal Projects/Night_of_the_Wolves/Night_of_the_Wolves.uproject" \
  /Game/NightOfTheWolves/Maps/M1_Trail -game -nullrhi -nosound -unattended -nop4 \
  -ExecCmds='t.MaxFPS 60' -abslog=/tmp/notw-m1-runtime.log
```

Exit 124 was the deliberate `timeout` after observing the loop, not a normal game exit.

## M2 blockout

The user reports that the M2 packaged playtest succeeded: the cabin encounter and ground correction worked as expected.

The editor script `Scripts/verify_m2_map.py` verified the saved floor reaches the perimeter boundaries, collision is enabled on the floor, walls, and door, and the door and hermit tags exist. It did not simulate a player or review the scene visually.

The editor target and Linux Shipping package built successfully. A headless editor-runtime regression check loaded the updated map, logged idle wolf capture at five seconds, and reloaded two seconds later. The M2 package is in `Saved/M2-Linux/Linux`. The user subsequently reported the cabin encounter and ground correction worked as expected; sound and presentation remain placeholders.

1. Run along the trail to the cabin. Approach the door until “Press E to knock on door” appears, then press E. The door should turn, a placeholder hermit should appear, and a placeholder grunt message should be followed by a shotgun death message and restart. Verify the door closes, the hermit disappears, and pursuit and input reset.
2. Run to the cabin and stop without knocking. Capture should occur after roughly five seconds, followed by the wolf death screen and restart. Pursuit continues during the hermit's reaction; either death may occur, but never both.
3. Check the ground and boundaries near and beyond the former trail end, and try moving around the blockout perimeter. Max should not fall into the void.

The cabin, window, shovel, hermit, and reactions are placeholders. The grunt and shotgun are currently text, without sound effects. The E and F keys are temporary keyboard bindings, not the final configurable control scheme.

## M3 prototype

The user reports the shovel solution and the ten-second bonus work, though ten extra seconds may be too much. They intend to fine-tune pacing later.

The user reported needing more time to take the shovel, dig, and knock, and requested a temporary ten-second extension on shovel pickup. The updated timing needs a fresh interactive playtest.

A headless editor-runtime `-M3ShovelTimeTest` physically approached the shovel, observed the calculated lead increase from 4.39 to 14.39 seconds, confirmed a repeated pickup action added no time, and then waited: wolf capture occurred after 14.40 seconds. This is an automated timing check; it does not establish whether a person has enough time to solve the encounter.

The updated Linux Shipping package was rebuilt and archived successfully to `Saved/M3-Linux/Linux`; the archived executable matches the staged executable. The new timing awaits an interactive packaged playtest.

An editor-runtime headless self-test with `-M3SelfTest -ExecCmds='t.MaxFPS 60'` moved the pawn physically along a route, invoked the shovel and door actions in code, and reached shelter in 10.28 seconds from the start. Pursuit was active throughout. This exercised collision, movement, the pit, hermit state and shelter transition; it did not verify keyboard bindings, visual presentation, alternative routes, or human reaction time. The test is excluded from Shipping builds.

The M3 Shipping build, cook and archive succeeded. A separate headless editor-runtime check observed idle capture at five seconds and a restart two seconds later, followed by another idle capture. The packaged build and the pit/reset cases still need interactive playtesting.

1. Run to the shovel at the right side of the cabin. Press F to take it. Move back toward the trail alongside the door and press F when prompted to dig. Stay to the side of the pit.
2. Approach the door from the side and press E to knock. The hermit should move out and fall into the pit. Go around the opening, enter the cabin, and press E when prompted to close the door. The shelter message should appear.
3. Repeat without digging and confirm the hermit shoots. Wait at the cabin or fall into the pit and confirm the wolves can still catch Max. Following death, verify the shovel, pit cover, hermit, door, movement, and prompts reset.
4. Assess whether the entire sequence can be discovered and completed fairly under pursuit. Check the pit is visible and the side route and door are navigable.

The pit, shovel, hermit movement and shelter message are placeholders. The five-second head start and instant interactions are provisional; no persistent checkpoint is created. The final red-eyed wolf sequence and ambient audio remain outstanding.

## M4 presentation pass

The user supplied a visual reference for a realistic moonlit, misty forest and weathered stone cabin. The first pass adds textured surfaces, a full-moon placeholder, cool lighting, mist, primitive branching tree silhouettes, public-domain wolf howls and wind, and a reusable 2D wolf-lunge overlay. The geometry and 2D lunge are placeholders and are not yet at the reference's realism; the hermit, shovel, and cabin shape remain blockouts. Check `Art/Source/Presentation/SOURCES.md` for asset provenance.

The editor target and revised M4 Shipping package built successfully. A renderer-backed, muted run recorded the fatal and shovel-to-shelter branches, and a headless physical-route self-test passed after the visual changes. A rendered starting-view screenshot was reviewed: the moon, trail, and cabin are visible under darker blue lighting, but the trees and cabin remain visibly primitive. Sound levels, loops, wolf-death image in the packaged build, and all packaged paths await interactive audiovisual playtest. Screenshot location (local build artifact): `Saved/Screenshots/LinuxEditor/HighresScreenshot00002.png`.

Open the project with the editor command in `AGENTS.md`, press Play, click the viewport to capture input, and use W/A/S/D and the mouse. The template moves at a running pace without a separate sprint button.

1. Leave all controls alone. After five seconds of game time, expect the placeholder “You died” overlay, then automatic restart after two seconds. Repeat twice. Check that Max starts at the same location and facing, the camera points down the trail, and movement and mouse look work after each restart.
2. On a fresh attempt, hold W down the light-colored trail. Pass between the two end markers and check for the “Trail end reached” message without a death. Pursuit remains active; the marker is not shelter or victory.
3. On another attempt, steer toward a blockout tree and hold movement into it. Check that the wolves eventually catch Max despite the held input. Repeat after moving the camera, and check the camera is restored after death.
4. Inspect trail readability and lighting. The gray blocks, template mannequin, light, end message, and text-only death screen are placeholders; the wolf lunge, forest art, moon, wolf howls, and forest ambience are not yet implemented.

Record observations and whether this was played in the editor or packaged build. The user-reported results above apply to the original package; retest after changes.

## Cases to exercise as implemented

1. Wait at the starting point: wolf capture after five seconds, shared wolf presentation, and restart at the original spot; repeat.
2. Run down the trail: remain ahead of the wolves and reach the cabin. Test stopping and blocked movement once pursuit behavior is defined.
3. Knock without a prepared hole: hermit grunts and shoots; death and restart restore the encounter.
4. Wait at the cabin: wolf capture and the same wolf presentation.
5. Take the shovel, dig, avoid the hole, knock, trap the hermit, enter, and shut the door. Measure pursuit timing throughout.
6. After each death, verify player position and facing, camera and input, shovel, hole, door, hermit, pursuit, prompts, and pending interactions reset as applicable.

Record the build or editor version, steps, observed result, and whether the test was performed in editor or in a packaged build when results become available.
