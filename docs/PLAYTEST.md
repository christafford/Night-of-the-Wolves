# Playtest record

## Title and wolf-eye opening, November 22, 2026

| Case | Observed result |
| --- | --- |
| Title and approach | The first rendered Development frame showed both title lines. The title held for 2.60 game seconds and dissolved over 1.45 seconds. The camera started at the beginning of the pursuit spline and ran 66.96 m along the curved approach. The final 1.70 seconds blended into Max’s live spring-arm view. |
| Handoff and retry | The opening driver ran at 20 and 120 fixed FPS. Both runs observed a moving, bobbing camera within 3.9 m of Max, then an idle pounce 5.00 seconds after handoff. The next attempt returned straight to third person. In a rendered run, Tab, Esc, E, F, R, jump, movement and mouse-look during the sequence left Max in place and the camera on its course. |
| Breathing | The Development stereo mixer captured 2.66 audible breath cycles per second alongside the camera’s 2.65 strides per second. In the 250–2900 Hz band the POV section measured about 16 dB above the forest before the dissolve. The panting faded out across the return to Max. [Recorded mix](../Saved/OpeningReview/Opening-Mix.wav) |
| Linux Shipping | BuildCookRun built, cooked, staged and archived the opening. The cook reported zero errors and warnings. UnrealPak confirmed the title, breathing and character assets, and the archived executable matched the compiled Shipping binary. A private CP1 save went directly to the cottage. Choosing New Trail twice played the opening twice. After the second handoff Max sprinted and the inventory wheel opened. The 1440 × 810 recording includes the game’s stereo mixer. Its running section measured 15 dB above the title section in the same frequency band. [Watch the opening with game audio](../Saved/OpeningReview/Opening.mp4) |

## Wolves and hermit appearance, November 22, 2026

| Case | Observed result |
| --- | --- |
| Pack closing on the trail | A Vulkan run used Max's third-person camera to look back at all three moving wolves. Their pale muzzles, dark brows and silver-edged ruffs remained visible as the flankers spread. A separate three-quarter detail view used the same forest lighting. [Moving approach](../Saved/CharacterArtReview/Wolf-Approach.mp4) · [Trail camera](../Saved/CharacterArtReview/Pack-Closing.png) · [Wolf detail](../Saved/CharacterArtReview/Wolf-Portrait.png) |
| Moonlight test | A light-on and light-off comparison with the revised coat sampled a 31 × 31 pixel square around each wolf's projected head just before the leap. On the leader and two flankers the median display luma was 79, 96 and 51 out of 255 with the dedicated reflection, compared with 28, 21 and 22 with it switched off. These are local screen measurements from the trail, taken before the final ear and eye pigment adjustment. |
| Hermit on the timbers | A 1440 × 900 Vulkan run captured his jog and head-up hold on the intact cover, then his fall into the dug hollow on the next knock. He reached both marks after 2.75 game seconds. The three-quarter detail view used the same warm cabin light. [Normal encounter framing](../Saved/CharacterArtReview/Hermit-Aim.png) · [Face and beard](../Saved/CharacterArtReview/Hermit-Portrait.png) · [Fall](../Saved/CharacterArtReview/Hermit-Fall.png) |
| Capture, retry and first leg | The Development capture driver observed the leader rise 1.05 m and put its paws within 13 cm of Max's chest before the death card. Automatic retry and retry during a second leap restored control. The clean movement driver reached CP5 with the expected inventory in 79.58 game seconds. The cottage recovery driver passed the unprepared knock, the dug-pit branch, the door breach and checkpoint restores. A separate doorway run exercised both unprotected exits and a lit departure. |
| Linux Shipping | The character-art build cooked with zero errors and warnings, and the archived binary matched the compiled executable. Three private-profile keyboard and mouse sessions checked the trail pounce and retry, the fatal knock from CP1, and the guarded exit and recovery from CP2. All three exited normally. The unaltered game windows were recorded with sound muted for these art checks. [Trail and porch footage](../Saved/CharacterArtReview/Trail-and-Porch.mp4) |

## Pack and cabin encounters, November 22, 2026

| Case | Observed result |
| --- | --- |
| Trot and flanking | A Development run sampled 296 frames per wolf. The leader’s forepaw swept 36 cm and lifted 16 cm. Diagonal paws moved together and opposing forelegs alternated. The flankers spread from 1.44 m apart to 3.59 m as the pack closed. Moving Max sideways changed their intercepts by 1.04 m and 1.56 m relative to the stationary run. [Pack footage](../Saved/EncounterReview/TimberPack.mp4) |
| Capture | In the trailhead Vulkan run the leader rose 1.05 m, planted his forepaws on Max’s upper body, and triggered his fall before the wolf death image. A headless check verified automatic retry and a manual retry during a second leap. Both restored the normal camera, input and a fresh pawn. The flankers finished their last steps and planted all eight paws. |
| Knock and timbers | Both knocks brought the armed hermit to the center of the same hollow, about 2.77 seconds after input. On intact boards he turned and held his aim for another 2.1 seconds before firing. Over the excavated hollow he fell, leaving Max free to enter. The two-knock recovery run also reached the barred-door checkpoint, retried it, and exercised a door breach. |
| Guarded exit | Approaching the open door from inside gave Max a view of the waiting wolves. An empty-handed departure and one holding an extinguished brand both triggered the pounce on crossing the door plane. Each restored the barred cabin at CP2 with the shovel equipped. The subsequent lit departure cleared the hollow. The recovery driver then reached CP5, including switching to rope at the creek. |
| Final Linux Shipping archive | BuildCookRun built, cooked, staged, packaged and archived with the [project command](../AGENTS.md) and `-AdditionalCookerOptions=-CookIncremental`. The cook reported zero errors and warnings, and UAT returned 0. The archived executable matched the built Shipping binary. UnrealPak listed the new wolf rig, coat, hermit head, cap, beard and jog in the cooked container. |
| Packaged keyboard play | The archived game loaded a copy of a barred-cabin checkpoint earned in the previous build. Normal keyboard input opened the door and backed Max across it with the shovel equipped. The pack was waiting outside. The leader sprang onto him, Max fell, the death image appeared, and the game automatically restored the cabin. The inventory wheel showed the shovel. The game exited with code 0. This run used a private profile with sound muted. [Door](../Saved/EncounterReview/Shipping-Waiting.png) · [Leap](../Saved/EncounterReview/Shipping-Leap.png) · [Impact](../Saved/EncounterReview/Shipping-Impact.png) · [Death](../Saved/EncounterReview/Shipping-Death.png) · [Retry](../Saved/EncounterReview/Shipping-Retry.png) |

## Playable Max, November 22, 2026

The Development Editor ran the full first leg with Max’s fitted outfit. A clean run reached CP5 in 78.57 game seconds, including the shovel, hearth, rope and gold. The recovery run exercised an early rope release, a second swing, wolf capture, checkpoint restarts and the final inventory. A skeletal probe found that the mannequin’s foot-placement graph was pinning Max’s feet during a run. With Max’s corrected locomotion Blueprint, 80 running samples showed 102.26 cm of fore/aft foot travel. Across the 1,375-sample recovery run, the observed offset between the matched head bones peaked at 0.03 cm during the swing. The recorded hand, foot and held-item attachment differences rounded to 0.00 cm.

A separate Linux Vulkan run used native hair and eyebrow strands. It reached CP5 in 85.05 game seconds with the gameplay camera enabled for screenshots. The hearth view shows the equipped brand in his hand, and the crossing view shows the sleeves and hands raised together on the rope. [Character review](../Saved/MaxReview/Max.jpg) · [Hearth](../Saved/MaxReview/Gameplay/03-Hearth.png) · [Rope swing](../Saved/MaxReview/Gameplay/06-Swing.png)

The final Linux Shipping archive built successfully. Its cook reported zero errors and warnings. An isolated profile launched the archived executable, ran Max forward with W, opened and closed the inventory wheel with Tab, and idled until the wolves caught him. The death screen appeared and the game automatically returned Max to the trailhead. The executable exited normally. [Running](../Saved/MaxReview/Gameplay/Shipping-Run.png) · [Inventory](../Saved/MaxReview/Gameplay/Shipping-Wheel.png) · [Capture](../Saved/MaxReview/Gameplay/Shipping-WolfCapture.png) · [Restart](../Saved/MaxReview/Gameplay/Shipping-Restart.png)

The cooked container includes the head, fitted outfit, native hair and brow bindings, and the updated fallback hair mesh and material. A separate Vulkan game run with strands disabled displayed the tapered hair cards and baked brows. Individual locks are more visible at portrait distance in this mode. [Fallback portrait](../Saved/MaxReview/Fallback.png)

## Moonlit Trail, November 22, 2026

The current first leg runs from the forest start (CP0) through the cottage approach (CP1), barred door (CP2), creek approach (CP3), far bank (CP4) and escape bend (CP5). The Shipping archive is `Saved/MoonlitTrail-Linux/Linux`. BuildCookRun built the Linux targets, cooked, staged, packaged and archived the project, returning exit code 0. The local launch command is in [README.md](../README.md).

| Case | Observed result |
| --- | --- |
| Final Linux Shipping archive | A fresh private profile began empty at the trailhead. After an initial opening retry to synchronize input, ordinary keyboard and mouse controls completed CP0 through CP5 in one attempt. The run took the shovel, trapped the hermit, barred the door, collected the supplies and brand, cast and released the rope, threw the gold onto the fixed patch and escaped. A fresh process loaded CP5. The wheel showed empty hands equipped, a shovel and an extinguished brand. This was the distributed archive with the final riverbank, bandit pose and wolf-reach changes. The save belonged to the test profile. |
| Clean route in editor game | The driver started empty, moved the pawn through the encounters, used the wheel, jumped the hollow, swung and released, threw the sack and escaped. The run reached CP5 in 78.65 seconds of game time. Final inventory contained the shovel and extinguished firebrand, with empty hands. |
| Cottage failures | An unprepared knock caused shotgun death and a CP1 retry with the pit covered and shovel available. The prepared route reached CP2 in 30.97 game seconds. Taking the firebrand before the grouped rope and gold pickup preserved the flame. Retry restored CP2 with the shovel and the supplies back in the room. Waiting indoors let the door breach after about 40 seconds, followed by wolf capture and another secured-cottage retry. This recovery check also passed with the renderer active. |
| Door cycling, missed supplies and relighting | The recovery driver opened the damaged door before taking supplies. The table reminder appeared, the bag stayed empty of rope and gold, and CP3 rejected the save. Closing the door retained 31.98 of 40 seconds. Switching away from the burning brand and returning to the hearth relit the same item and retained 32.98 of 36 seconds of wolf hesitation. |
| Inventory and pursuit | The development test drove the controller’s wheel bindings and checked that previewing a selection held game time and equipment steady. Canceling preserved the burning firebrand. Confirming the rope extinguished it. |
| Crossing and robbery failures | Walking away during a cast preserved the rope. An early release caused current death and CP3 restored both supplies with the rope unattached. A second attempt deliberately left the bank late. The wolf reached takeoff while Max was already over open water, and Max landed safely. Rushing the gunman caused a fatal shot. An off-target throw preserved the gold. After a valid throw, lingering beside the pack caused death and CP4 restored one coin sack and the world rope. The driver then escaped with the expected inventory. |
| Earlier optional-firebrand behavior | Before the doorway guard was added, a CP0–CP5 recovery run could skip the hearth pickup and finish with only the shovel. |
| Persistence in separate processes | A development process wrote CP2 to an isolated test slot. A fresh process resumed at the saved position, with shovel equipped, door barred, pit open and hermit trapped. The probe removed its test slot after validation. |
| Shipping keyboard and mouse | A muted X11 run of the first Shipping package opened the wheel, waited over six seconds, canceled, observed wolf capture and retry, then ran the approach and equipped the shovel. Actual game input completed the prepared trap and barred the door. Relaunching the executable resumed CP2 with the shovel. Captures are under `Saved/FirstLegReview/Shipping`. This run preceded the final art and swing-camera changes. |
| Shipping from the cottage to the bend | A second muted Shipping session resumed that earned CP2 with the revised swing camera. Keyboard and mouse input took the burning brand, collected both supplies, opened the door, jumped the pit, and reached the creek. Canceling the wheel preserved the fire. Confirming rope selection extinguished it. Cast, grip and release landed Max at CP4. Looking back brought up the gold target. Throwing and running past the bandit displayed “Moonlit Trail complete” and wrote CP5. The wheel showed empty hands and the two retained items. Both sessions used an isolated user directory. Captures 12–22 show the route. Subsequent changes refined the riverbank, the gold patch and the wolf's reach at takeoff. |
| Presentation | Renderer-backed development runs produced views of the trail, cottage, hearth, wheel, creek, swing, bandit and escape bend under `Saved/FirstLegReview`. The cabin window and roof read from the approach, and warm firelight reaches the interior. A separate recovery run used the SDL3 realtime mixer. Character models, staged wolf motion, effects and synthetic dialogue remain provisional. |
| Final picture and sound | The [90-second walkthrough](../Saved/FirstLegReview/Moonlit-Trail.mp4) shows the connected route from empty hands through CP5. It records the development game's own camera, controls, HUD and stereo mixer. This review run looked back during the diversion and reached CP5 in 85.32 game seconds. The corrected soil and shingle shader is visible on the creek banks. The final cook reported zero errors and zero warnings. A [six-view gallery](../Saved/FirstLegReview/Moonlit-Trail.jpg) shows the trail, turnoff, hearth, swing, bandit and exit. |

The development movement tests exercise player collision and game rules. The drivers steer toward known targets and invoke interactions programmatically. Their elapsed times describe a practiced route. The separate Shipping runs sent keyboard and mouse input to the game window. An uncapped 1080p Epic Shipping run on GPU-accelerated Weston/Xwayland averaged 92.64 FPS on the opening approach, with a 16.34 ms 99th-percentile frame time on the local RTX 3080. The [local profiling report](../Saved/FirstLegReview/Frametimes/README.md) records presentation overhead on Xvnc, hardware and the raw samples. Discoverability on a first visit, gamepad feel, dialogue and ambient balance, and frame times across the remaining encounters and other machines remain open playtest questions.

## Repeat the checks

1. Use the clean-run editor command in [AGENTS.md](../AGENTS.md). Re-run with just `-FirstLegTest` for recovery and with `-FirstLegTest -OmitFirebrand` for the route without the fireplace pickup. Check for the final pass message as well as a successful exit.
2. Use `-CottageRecoveryTest` for the door-breach and cottage restore branches. The development disk check requires **two** game launches. Use `-M3SelfTest -FirstLegSaveProbe` on the first and `-FirstLegSaveProbe` alone on the second. Omit `-NoTrailSave` on these two runs. Look for `DISK CHECKPOINT TEST PASSED`. The probe uses a separate save slot.
3. For a human playthrough, launch the Shipping build with sound. Inspect the cottage reveal, switch away from the firebrand using the wheel, miss a swing on one run, and attempt the gold toss while looking away before aiming at the marked ground. Verify the held item, prompt, visible pack position, feedback and restored checkpoint in each case. Record hardware, resolution, frame timing and any moment where the player loses sight of a hazard or destination.

## Historical prototype records

The following observations, instructions, archive paths and outstanding questions describe earlier builds at the dates they were recorded. The current route, controls, checkpoints and acceptance results are above.

<details>
<summary>Original baseline through the first procedural forest pass</summary>

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

## Longer winding approach

The user requested a substantially longer, curved trail with rises and dips, an initially hidden cabin revealed after a bend, a lit window, and a fireplace inside. A spline now drives pursuit progress along the route; the cabin puzzle keeps its existing location and rules. The earlier prototype's trail-end text is removed to preserve the initial uncertainty.

An automated headless run followed the physical route and completed the shovel-to-shelter path in 41.95 seconds with pursuit active. A separate idle check recorded capture at five seconds and restart. Rendered editor screenshots were inspected at the start and after the bend: the cabin was hidden initially and its warm window was visible from the later position. An interior screenshot caught an excessively bright light; after adjustment the interior was warm and no longer washed out. Fire, fireplace, terrain, trees, and cabin remain prototype geometry. These screenshots and automated actions do not establish human navigation, camera usability, real-time reveal timing, sound mix, or packaged gameplay; those require a user playtest.

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

## Middle start, side cabin, and procedural forest pass

The user confirmed the previous winding approach worked, then requested a start midway along it, a continuing trail past the cabin, denser varied trees, and a more natural path. The current prototype makes those layout changes and adds a first PCG graph. PCG uses randomized instances of a provisional low-poly dead-tree mesh among the existing primitive trees; the Procedural Foliage Tool has not yet been used to populate the map. The ground, path edges, trees, cabin, fireplace and fire remain blockout presentation.

A headless editor-runtime continuity test physically ran the route past the cabin to the end with pursuit active in 33.75 seconds; PCG logs reported 116 dead-tree instances generated across 29 patches. A separate automated editor-runtime test with PCG and the extended forest floor completed the shovel, pit, hermit and shelter route in 25.18 seconds. A rendered starting screenshot was inspected; the trail is readable and the cabin is hidden, but the cone trees and exposed horizon remain visibly primitive. These checks do not establish that a player can navigate and discover the cabin under pressure, that the PCG forest looks natural, or that the packaged game performs well.

Interactive playtest still needed: look both ways from the start; follow the trail and inspect the reveal and side spur; complete the shovel-and-pit solution; then continue past the cabin on a separate attempt. Check for visible gaps, floating trees, obstructions, lighting and performance. There is intentionally no new blocker beyond the cabin yet.

The final revised map was rebuilt, cooked and archived successfully to `Saved/Forest-Linux/Linux`. Its Shipping process stayed running during a 15-second `-nullrhi -nosound` launch smoke check; Shipping emitted no console log, so this does not verify map loading, PCG generation, visuals, or gameplay in the package. Interactive packaged verification remains pending.

</details>
