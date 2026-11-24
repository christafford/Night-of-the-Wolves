# Playtest record

## The robbery and “No Birds at This Hour”, November 23, 2026

The Development check `Wolf.TestRobbery` started with an earned far-bank save. It threw with the sack equipped, used the gameplay sprint input, and tracked Max's movement, the pouch landing, the wolf strike and the saved checkpoint.

| Attempt | Observed result |
| --- | --- |
| Throw uphill and sprint past the robber | Max ran 11.88 m at up to 6.75 m/s. The bandit shot him 1.93 s after release. |
| Throw across the trail and sprint along the outer shoulder | Max ran 11.93 m. The bandit shot him 2.58 s after release. |
| Drop the sack short and stay still | Max stood where he threw it. The bandit shot him after 2.27 s. |
| Throw into the pack’s approach and immediately bolt | The pouch landed on the approach. The bandit was still armed. He shot Max after 1.93 s. |
| Wait for the takedown, then sprint | The leader struck the robber. The camera returned to Max and the sprint earned CP5 on disk. |

In the final Linux Shipping build, keyboard and mouse input sent Max sprinting around the bandit immediately after the wrong throw. The bandit’s shot brought up the death card 2.10 s after release. Retry restored the pouch. In the same process, a throw toward the pack, the actual takedown and a sprint after camera handoff earned CP5 on disk. Nine passages in the packaged stereo mix matched the new score at a common offset. The game exited 0. [Watch both packaged attempts with game audio](../Saved/RobberyReview/No-Escape-Without-the-Pack.mp4)

The 76-second stereo score was reimported as a looping Unreal SoundWave. A 96.8 s capture of the Development game mixer at the burning tree matched twelve passages from the new master, including a 1.25 s excerpt across the loop boundary. The stereo source measures −21.0 LUFS and −6.12 dBTP. [Hear the new score](../Art/Source/Music/SW_MoonlitPursuit.wav)

Each death returned Max to CP4 with exactly one sack. Early requests to save CP5 were refused. A separate clean run reached CP5 in 99.50 game seconds and continued through the storm and cave to CP6. The full recovery run also passed. Linux Shipping compiled, cooked, staged and archived successfully in 3m 39s. The archived executable matched the built binary, and all 81 checked assets were in the cooked container.

## Previous release introducing the storm at the escape bend, November 23, 2026

| Case | Observed result |
| --- | --- |
| Opening and wolf capture | The Development camera ran 67.5 m past four wolves, revealed the same silver alpha taking the lead, and returned control to Max after 11.23 s. An idle Max was pounced on 5.03 s after handoff. The recorded card snarl starts as the static death image appears. Retry returned to third person. |
| Packaged opening and death-card audio | A 21.7 s stereo recording from the archived executable shows the four runners, the silver fifth taking point, the handoff, pounce, static portrait and CP0 retry. Three excerpts from the snarl matched the game audio, starting within 0.10 s of the first visible death lettering. The score enters on the handoff. [Watch the opening and capture](../Saved/StormReview/Alpha-and-the-Pack.mp4) |
| Miss and recover at the robbery | The full movement and equipment driver threw uphill. The sack landed 5.44 m away. The bandit went to it, the wolves caught Max, and CP4 restored one sack. Two throws downhill landed 2.26 m apart across the approach. Both brought the wolves to the bandit. Lingering through the distraction let the pack return to Max. Running on earned CP5. |
| Packaged gold throw | With keyboard and mouse in the archived game, a throw uphill spent the pouch. The wolves caught Max, automatic retry restored CP4 and the equipment wheel offered the sack again. A throw downhill drew the bandit into the live pack. The close-up, takedown and camera handoff played, and Max reached CP5 on disk. The process exited 0. [Watch both throws](../Saved/StormReview/Two-Gold-Throws.mp4) |
| Fall into the river | Starting at the near bank, Max walked into the current twice. The existing Wilhelm recording started 0.17 s after entering the water. His wrist travelled 96 cm, his head descended 90 cm and the current carried him 2.57 m. The card appeared after 3.40 s. Automatic retry and a retry during the struggle restored the rope, gold, camera and movement at the near bank (CP3). |
| Packaged river and score | The Shipping player sprinted off the near bank using W and Shift. The stereo recording shows Max’s alternating reaches, submersion, card and respawn with his original burning brand. The captured mix matched the Wilhelm recording and 20 passages of the new brass-and-drum score. The process exited 0 and the isolated save remained at CP3. [Watch Max in the current](../Saved/StormReview/Max-in-the-Current.mp4) |
| Wolves fear the burning fir | On a continuous trailhead run, the five original wolves increased their distance from the fire by 9.20, 9.91, 10.65, 15.44 and 5.12 m. Two started off the road near the river. The test followed the same actors frame by frame and rejected discontinuous movement. A second run retried during the reaction shot, replayed it from CP5, then left Max standing in the clearing without a flame for 64 s. All five stayed in cover and Max retained control. |
| Carry fire through the cave | An unlit brand and forward movement left the roots intact. The fallen fir lit the carried brand. Stowing it put it out. After relighting, Max touched it to the roots and walked through the cleared side passage to CP6. A scratch copy of a legacy loadout containing only the shovel also succeeded by taking a burning branch at the trunk. |
| Packaged strike and cave | Keyboard and mouse play from the archived CP5 preview showed the tree split, the pack flee and the view return to Max. He equipped the old brand, relit it, burned through the roots and walked into the next chamber. The resulting on-disk save was stage 6. A fresh Development process loaded that exact Shipping save with Max grounded inside, the brand lit, roots open and the tree still blocking the road. Both processes exited 0. The recorded stereo mix contains the thunder, splintering fir, impact, crackling wood and catching roots. [Watch the Shipping playthrough](../Saved/StormfallReview/Stormfall.mp4) |
| Save compatibility | A real outlet save from the previous grate build reopened at CP5 with its carried items. The new inner-chamber save reopened at CP6 in another process with intact floor collision, a burning brand, open roots and the fallen fir still across the road. An earlier CP2 save probe also passed a disk round trip. |

The Linux Development editor compiled successfully. Final Linux Shipping built, cooked, staged, packaged and archived in 3m 39s with exit 0. The archived executable matched the compiled Shipping binary byte for byte. The IoStore listing contained all 81 checked packages, including the score, snarl, storm scenery and storm audio. The retired culvert directory was absent.

The Development verification used the startup map with normal movement, the inventory wheel, collision and checkpoint rules. From the project folder, these are the build and launch commands for this revision.

```sh
export UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
UE_PROJECT="$PWD/Night_of_the_Wolves.uproject"
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" Night_of_the_WolvesEditor Linux Development \
  "-Project=$UE_PROJECT" -NoUBA -WaitMutex
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  "-project=$UE_PROJECT" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -package -archive "-archivedirectory=$PWD/Saved/MoonlitTrail-Linux" \
  -AdditionalCookerOptions=-CookIncremental -ubtargs=-NoUBA -utf8output -unattended
./Saved/MoonlitTrail-Linux/Linux/Night_of_the_Wolves.sh
```

## Earlier build with the retired Singing Culvert, November 23, 2026

| Case | Observed result |
| --- | --- |
| Previous build’s escape-bend save | A copy of the earlier Linux CP5 save reopened at the bend with the shovel and extinguished firebrand in the bag. An unarmed Max could neither lift the rack nor walk through it. The outlet rejected an early checkpoint request. |
| A quiet escape | The Development driver selected the shovel through the inventory wheel and used real press and release input at the grate. Four catches stayed in place and Max walked the tunnel to CP6. A separate uninterrupted run reached CP5 from the trailhead in 91.58 simulated seconds and cleared the culvert a further 38.20 seconds later. |
| Make a mistake, go still, then keep making noise | The actual tracker approached to 10.8 cm of its inspection mark, looked through the wing and left Max alive when he stopped. One catch held after release. Continuing to scrape on another pass drew the wolf around the wing and it earned a clear pounce from 2.15 m. Automatic retry restored the original bend inventory, lowered the grate and cleared the alert. The same recovery run reached CP6 in 51.25 simulated seconds. The final ringing catch brought the tracker back to within 11.8 cm of its inspection mark before Max cleared the outlet. |
| Outlet on disk | A fresh Development process opened the CP6 saved by the recovery run, with the shovel retained, a clear collision trace through the raised gate, and Max grounded beyond the outlet. |
| Moonlight | Matched, rendered player cameras showed a 26% reduction in mean trailhead image luminance. The bright end of the upper sky on the cabin approach fell from 167 to 60 on a 0–255 scale. The warm hearth interior stayed close to its previous exposure. A real opening in the canopy lets the moon reach the forebay. The closed rack, spray, Max and the ribbed pipe remained readable with ordinary dynamic shadows. [Compare the trail and cabin](../Saved/CulvertReview/Trail-Moonlight.jpg) |
| Packaged Linux input and audio | The final Shipping executable loaded an isolated copy of the old bend save. Using the mouse, WASD, inventory wheel and F, Max investigated the grate, stopped after an exposed scrape, made four timed strokes and walked from the inlet through the outlet. The file on disk contained CP6 and the game exited 0. The separate bar tap, rain, shovel scrape and ratchet were identifiable in the stereo recording. [Watch the 54-second puzzle playthrough (spoilers)](../Saved/CulvertReview/The-Singing-Culvert.mp4) |

The first four cases used the normal startup map with a fixed 60 Hz clock and an isolated save directory. The lighting comparisons used the game renderer. The culvert contains original geometry and synthesized local foley. The four sounds and the authored meshes were saved through Unreal Editor. Gameplay loads the cooked assets.

The Linux Development editor compiled successfully. BuildCookRun built, cooked and archived Linux Shipping with exit 0 in 3m 56s. The cooked container includes all 14 culvert assets, the moonlit sky and the startup map. To repeat the archive or play the section directly, use these commands from the project root.

```sh
"$HOME/UnrealEngine-5.9-preview-2/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  "-project=$PWD/Night_of_the_Wolves.uproject" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -package -archive "-archivedirectory=$PWD/Saved/MoonlitTrail-Linux" \
  -AdditionalCookerOptions=-CookIncremental -ubtargs=-NoUBA -utf8output -unattended
./Scripts/play_culvert.sh
```

## Under the Boughs and first-leg prompts, November 23, 2026

- The editor imported the new looping SoundWave with zero import errors or warnings. Two independent renders produced the same stereo master, exactly 3,072,000 frames at 48 kHz. FFmpeg measured −19.01 LUFS integrated and −4.61 dBTP. At the loop boundary the larger of the two channels’ sample jumps was 0.0023 on a full-scale range of ±1.
- In the archived Linux Shipping game, a fresh profile went from the title through the wolf camera into a keyboard-and-mouse run until the cabin light came into view. Ten passages from the new master matched the recorded stereo output. The score entered about 8.3 seconds into the clip as control returned to Max and reached its settled level 2.25 seconds later. The title and wolf camera were free of the score. Wolf breathing matched during the approach and 31 trail-footstep contacts matched during the run. The game exited normally. [Watch the packaged opening with game audio](../Saved/TrailPerformanceReview/Forest-Pulse-Opening.mp4)
- The Development opening test held the score through the title and wolf camera and selected the looping cue on the handoff at 10.10 seconds. With Max standing still the attack began 5.02 seconds later. Retry returned directly to Max. This test ran with rendering and audio output disabled.
- The Development clean route reached the escape bend in 91.58 simulated seconds. The cottage recovery case opened the door first with neither supply and then with only the rope. In both cases the door opened without an inventory reminder. The incomplete inventory did not advance the checkpoint. The full recovery run released the trolley over the current, restored the near-bank inventory, tried a bad gold throw, then completed the leg. These cases checked gameplay and prompt state with rendering and audio output disabled.

The Development editor build succeeded. The editor WAV import and Linux Shipping archive used the project root as the working directory. BuildCookRun finished in 3m 34s, exit 0.

```sh
ue="$HOME/UnrealEngine-5.9-preview-2/Engine"
project="$PWD/Night_of_the_Wolves.uproject"
"$ue/Build/BatchFiles/Linux/Build.sh" Night_of_the_WolvesEditor Linux Development \
  "-Project=$project" -NoUBA -WaitMutex
"$ue/Binaries/Linux/UnrealEditor-Cmd" "$project" -run=pythonscript \
  "-script=$PWD/Scripts/score/import_moonlit_pursuit.py" -nullrhi -nosound -unattended -nop4 \
  -abslog=/tmp/notw-forest-pulse/score-import.log
"$ue/Build/BatchFiles/RunUAT.sh" BuildCookRun "-project=$project" -noP4 -platform=Linux \
  -clientconfig=Shipping -build -cook -stage -pak -package -archive \
  "-archivedirectory=$PWD/Saved/MoonlitTrail-Linux" -AdditionalCookerOptions=-CookIncremental \
  -ubtargs=-NoUBA -utf8output -unattended
```

## Five-wolf pursuit, November 23, 2026

| Case | Observed result |
| --- | --- |
| Opening and escape | A fresh trail released Max after the 10.10-second opening. With no input the pressure wolf earned a clear pounce 5.02 seconds later. Its paws reached within 13 cm of Max’s chest before the card appeared. Automatic retry and a retry during the leap both restored control. A separate clean run reached CP5 in 91.58 simulated seconds. [Watch from Max’s camera in Linux Shipping](../Saved/FirstLegReview/Wolves_at_the_Trailhead.mp4) |
| Narrow and open ground | In a collision-backed Development fixture the five possessed pawns progressed through the narrow trail with a minimum 1.45 m between centers. In the clearing, a flanker traveled 16.41 m on its approach while a second wolf kept direct pressure. The coordinator signaled the change. |
| Ordinary far-bank route | The game’s existing creek stones and ferns separate an 8.80 m direct path and an 11.52 m alternate. Both paths passed ground and body-clearance checks. After the final cornering adjustment, the interceptor traveled 4.02 m on its chosen route and reached 3.05 m of lateral separation before Max reversed and it canceled the intercept. The recording shows the same maneuver with the earlier cornering tune. [Watch the pack on the trail and at the far-bank fork](../Saved/FirstLegReview/Five_Wolf_Pursuit.mp4) |
| Hidden Max and blocked wolves | A solid screen hid Max while he changed position. The last sighting stayed fixed for 5.20 seconds, the tracker investigated and the pack reacquired Max after the screen came down. Closing both lanes of a separate fixture invalidated both routes. An unsupported gap also failed the route check. A separate 60-second loss of all sight and sound took the search 52.78 m beyond the frozen observation. The tracker returned to check the last-known location seven times. |
| Three-minute pursuit and retries | Over 180 simulated seconds, pressure changed hands on each of three forced obstructions. The pack made 1,800 staggered decisions and 1,797 route requests. At most one wolf held a lethal reservation. Repeated checkpoint loads retained the same five possessed pawns and cleared transient state. A separate cold launch restored a previously saved CP2. The main route also passed an early trolley drop, a late departure, the creek camera interruption, the bandit shot and the diversion-death retry. |
| Cottage protection | Bare hands and an extinguished firebrand each produced a doorway capture followed by a secure CP2 retry. A burning brand allowed the jump and departure, followed by a run to CP5. Another run left Max idle inside. All five stayed outside for the full 40 seconds the door held. After the bar fell, the pressure wolf cleared the pit and launched from 1.21 m away. Automatic retry put Max back inside with the bar up and all five wolves outside. |

The final Linux Shipping cook reported zero errors and zero warnings. The archived binary matched the compiled Shipping target, and Unreal’s container listing found all 62 assets checked for the first leg, including the score, footfalls, voices and cable hardware. A fresh disposable-profile launch ran the title, chase, pounce, death card and retry. A second standalone run resumed a copy of CP3, cast the rope, rode across, watched the wolves and walked off the far platform. Its save advanced to CP4. Both Shipping processes exited normally. The pursuit comparison uses a Development observation camera and brighter review exposure so the dark wolves remain visible through the trees.

The [pack implementation and tuning guide](WOLF_PACK.md) lists the live debug view and repeatable Development checks.

## Voices, Max, the robbery and the trolley, November 23, 2026

| Case | Observed result |
| --- | --- |
| Actor performances | All six re-recorded voices matched the captured game audio. The hermit’s warning grew from 1.53 to 2.81 seconds and its median voiced pitch dropped from about 152 to 97 Hz. In a rendered run the hermit finished his plea before he fell, and the bandit’s three lines played in sequence before the pack reached him. The 50 Hz mouth data fitted each mastered clip. All 26 scripted words were recovered from the isolated masters. The bandit’s accent and both performances’ dramatic effect are listening judgments. [Hear the lines in the game](../Saved/VoiceReview/In-game-dialogue-Revised.wav) |
| Digging and running | Ordinary Shipping input produced the three two-handed spade strokes, displaced earth and opened the hollow. The spade stayed equipped. The Shipping mix matched distinct trail-footstep recordings while Max ran. Recorded Shipping output independently matched timber hits while Max ran inside the cabin. Ground-contact logging selected wood on both trolley decks and stopped while Max was on the cable. [Watch Max run and dig with game sound](../Saved/FirstLegReview/Max_Dig_and_Footfalls.mp4) |
| Gold | The Shipping HUD read “F  Throw gold” when facing the bandit and when looking back down the road. The naturally lit clearing had no target marker. The native recovery case retained the pouch on a bad aim, spent it on a successful throw and restored exactly one on retry. |
| Cableway | In Shipping, a normal jump under the bare hook fell short. The saddle is about 4 m above the deck and Max’s jump lifts him 1.28 m. He cast the rope overhead with F, gripped the low sling, crossed and dismounted on the far planks. [Watch the jump, cast and ride](../Saved/TrailPolishReview/Trolley-Cast.mp4) |
| Score handoff | The 108 BPM cue is a 71.11-second original loop. A waveform match against the packaged stereo mix found its entry as control returned and its fade up over the following two seconds. Title and wolf POV samples had no matching score. [Watch the opening with the new score](../Saved/TrailPerformanceReview/Opening.mp4) |

The Editor and Shipping verification runs used the source engine at `$HOME/UnrealEngine-5.9-preview-2`. These commands run from the project root.

```sh
export UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
UE_PROJECT="$PWD/Night_of_the_Wolves.uproject"
"$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh" Night_of_the_WolvesEditor Linux Development -Project="$UE_PROJECT" -WaitMutex -NoUBA
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$UE_PROJECT" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -package -archive -archivedirectory="$PWD/Saved/MoonlitTrail-Linux" \
  -AdditionalCookerOptions=-CookIncremental -ubtargs=-NoUBA -utf8output -unattended
"$PWD/Saved/MoonlitTrail-Linux/Linux/Night_of_the_Wolves.sh" -NoTrailSave
```

## Creek lookback and pursuit score, November 23, 2026

| Case | Observed result |
| --- | --- |
| Fresh trail | The opening test observed the title dissolve and wolf camera for 10.10 seconds. Max had 5.02 seconds before the first idle pounce. In Linux Shipping, the captured stereo mix first matched the music at the handoff, reaching the configured level over 2.2 seconds as Max ran down the trail. Earlier title and wolf-camera audio had no matching music. [Watch the opening with game audio](../Saved/CreekScoreReview/Opening-Score.mp4) |
| Dismount and handback | Both a moving release and a parked release reached the far planks. The live lead wolf stopped at the broken span for about 1.5 seconds, then moved downstream and across the channel. The view returned to Max with movement and mouse look restored. The clean route continued through the bandit to the final checkpoint in 95.47 simulated seconds. |
| Retry during the shot | A separate run retried while the wolves were hesitating. The far-bank checkpoint restored Max’s camera and controls, with the rope still attached to the trolley. In Shipping, choosing Retry Checkpoint from the pause menu during the wide shot also returned immediately to Max on the far bank, and he walked onto the road. The regular recovery run passed the current death and subsequent bandit retries. |
| Music loop and retries | A sound-enabled Development run resumed an older near-bank save and idled through two deaths and restores. The stereo output matched the master across its 72-second loop boundary, including the returning opening phrase. The score kept its place through both retries. |
| Packaged creek | Keyboard and mouse dismounted the trolley, watched all three wolves reach open water, and walked Max off the landing after the handback. A separate cold launch from an older far-bank save gave control directly to Max. Both exited normally. The final Linux cook reported zero errors and warnings, and the archived Shipping executable matched the compiled binary. [Watch the far-bank lookback with game audio](../Saved/CreekScoreReview/Creek-Lookback.mp4) |

## Logging cableway at the creek, November 22, 2026

From the project directory, `./Saved/CablewayReview/Play-Crossing.sh` starts the packaged game at the near bank. Each launch gets a disposable copy of the earned creek checkpoint.

| Case | Observed result |
| --- | --- |
| Loading stage | The near-bank checkpoint from the previous Linux build initially overlapped the new ramp. Shortening the ramp cleared the saved spawn. Normal keyboard input from that checkpoint reached the iron eye, switched from the firebrand to the rope, threaded the sling and started the ride. |
| Equipment and the current | The native recovery run tried the empty trolley with the firebrand, stepped away during threading and kept the rope. A completed sling spent the rope once. In Shipping, E over open water dropped Max into the current and displayed “You died.” The near bank reloaded with the empty trolley, rope in inventory and burning firebrand equipped. A second ride from that same session reached the far bank and saved. |
| Far stop and pursuit | One native run released onto the landing while moving. Another departed with a wolf at the bank, held on at the far buffer for 1.22 seconds, then used E to dismount. The pack took the downstream water route. A later death restored the far-bank checkpoint with the trolley parked across the creek and the gold still available for the bandit. |
| Hands, framing and footing | The final Vulkan view from the ordinary ride camera showed the complete turning wheel, both hands on the sling, Max’s boots and the far landing. After trimming the road foundation and raising the planks, both the moving and parked releases grounded the capsule 2.2 cm above the board surface. The soles met the planks in the rendered view. |
| Full first leg | The clean Development run reached the escape checkpoint in 87.37 simulated seconds. The separate recovery run passed the interrupted knot, current death, far-side dismount, two bandit retries and final inventory checks. |
| Linux Shipping | The creek replay launcher used a copy of a checkpoint earned with the original tree crossing. Keyboard and mouse equipped the rope, rigged the sling, rode to the far stop and dismounted onto the planks. The save advanced to the far bank. A separate cold launch of an older far-bank save restored the trolley and rope across the water, and Max walked away. Both sessions exited normally. The game audio recording contains the shackle clink and end-stop impact. [Watch the packaged crossing with sound](../Saved/CablewayReview/Crossing.mp4) · [Trolley detail](../Saved/CablewayReview/Trolley-and-Sling.jpg) |

## Log cabin, separate supplies and the bandit’s takedown, November 22, 2026

| Case | Observed result |
| --- | --- |
| Cabin and yard | Vulkan renders show the notched wall logs, overlapping roof shakes, ledged plank door and ironwork under the existing porch and fire light. The T-handled spade leans beside a hooped oak barrel and a bound hay bale. Only the spade disappears on pickup. A separate editor check measured the visible leaf against the moving door blocker at 0°, 27°, 90° and closed again. [Cabin exterior](../Saved/HomesteadReview/Cabin-Outside.jpg) · [Door](../Saved/HomesteadReview/Cabin-Door.jpg) · [Yard](../Saved/HomesteadReview/Cabin-Yard.jpg) |
| Bare-handed knock | Normal keyboard input in Linux Shipping took and equipped the shovel and dug. Pressing E with the shovel held kept the door closed. Selecting Bare hands in the wheel changed the prompt to “E Knock.” Max knocked, watched the encounter, jumped inside and barred the door. The private CP1 save advanced to CP2. The intact-cover knock and CP1 retry also passed the native recovery test. [Packaged input footage, silent](../Saved/HomesteadReview/Shipping-Shovel-and-Knock.mp4) |
| Separate supplies | In the final Linux package, E took the sack from the worktable. A second E at the table left the rope visibly hanging on the opposite wall. Max walked to the peg and took it with a separate E. Each pickup showed its own confirmation and both names appeared separately in the wheel. The native recovery run also collected the rope first, kept the equipped firebrand and got a warning about the missing sack at the exit. The next checkpoint required both items. [Packaged pickups, silent](../Saved/HomesteadReview/Shipping-Cabin-Supplies.mp4) · [Firelit room](../Saved/HomesteadReview/Cabin-Room.jpg) |
| Bandit | In the Vulkan recording the pack was about 5.2 m from the bandit when he reacted. The close-up showed the change from greed to alarm. The view widened from 31° to 57° as the lead wolf leapt and the flankers closed. The stereo mix contained the Wilhelm scream at contact. His chest settled 21 cm above the ground. The camera and input returned to Max after the struggle. An independent Shipping run used keyboard and mouse to throw the sack, watch the attack, resume sprinting and advance a private CP4 save to CP5. [Bandit takedown with game audio](../Saved/HomesteadReview/Bandit-Takedown.mp4) |
| Progression and saves | The clean Development route reached CP5 in 87.82 simulated seconds. The failure/retry route and both guarded-door failures passed. Fresh processes restored both an earlier barred-cabin save with the shovel equipped and a newly earned save with the shovel stored and bare hands equipped. |
| Linux Shipping | BuildCookRun built, cooked, staged and archived the game to `Saved/MoonlitTrail-Linux/Linux`. The cook reported zero errors and warnings. The archived executable matched the compiled Shipping binary and all 39 expected map, art and audio packages were present. The keyboard playthrough of the firelit room exited normally. |

## Speaking faces and the hermit’s fall, November 22, 2026

[Watch the two encounters with recorded game audio](../Saved/FacialReview/Dialogue-Encounters.mp4)

| Case | Observed result |
| --- | --- |
| Warning and doorstep | The window warning kept Max’s 90° gameplay view. A physical knock on intact timbers brought the hermit outside, showed his shout in a 30° close-up, then produced the shot and death card. On the dug route, a 33° view from below his hat brim showed both eyes and his moving mouth during “Get me out of here!” The measured mouth motion in both lines was within 20 ms of the recorded Linux audio output. |
| Prepared pit | The game audio recording contains the plea, the original Wilhelm scream and one shotgun discharge. The shot followed the start of the scream by 1.24 seconds. The camera showed his upper body and arms inside the pit. The recorded shot landed during the visible flash. He remained motionless for over two seconds after impact, with no further dialogue. The clean run jumped the hollow, barred the door and proceeded to the escape bend. |
| Roadside gold | In the Vulkan game, Max opened the inventory wheel, showed the gold, turned to the patch and threw it. The camera framed the demand, the gold response and the final order. The bandit changed expression when the sack appeared. His three visible lines tracked the stereo output within 5–25 ms. |
| Recovery | The clean first-leg driver reached CP5 in 84.02 simulated seconds. The separate failure driver recovered after a missed swing and a failed gold diversion. The cottage driver verified the unprepared knock and CP1 retry, then secured and restored CP2. |
| Packaged cottage | Starting from a private copy of the earned CP1 save, normal keyboard input took and equipped the shovel, dug at the side of the door, knocked, and jumped the open hollow after the accidental shot. The bar-door prompt appeared inside. Barring the door wrote CP2 to the private save. The game exited with code 0. The stereo recording matched all five expected cues, with the gunshot 1.26 seconds after the scream began. [Packaged cabin playthrough](../Saved/FacialReview/Shipping-Cabin-Pit.mp4) |
| Linux Shipping | BuildCookRun built, cooked and archived the Linux executable with zero cook errors or warnings. The archived executable matched the compiled binary. The cooked container included both moving hair meshes, the shared mouth material, all six spoken cues, the fall scream and the shotgun report. A private far-bank save exercised the demand, early gold presentation, fatal shot and CP4 retry with normal keyboard and mouse input. Comparing mouth openings in 30 fps footage to the recorded output put both bandit lines within about 40 ms. A second run equipped the gold under the bandit’s gaze, turned out of the close-up and threw it with 1.36 seconds left in his response. Its final syllable and the queued parting order both played. [Mouth shapes](../Saved/FacialReview/Shipping-Bandit-Words.jpg) · [Packaged gold toss](../Saved/FacialReview/Shipping-Bandit-Gold.mp4) |

## Hermit and bandit dialogue, November 22, 2026

| Case | Observed result |
| --- | --- |
| Finished performances | Blind English transcription with whisper.cpp small.en recovered the words of all six mastered cues. The clips run 1.03–3.01 seconds. Integrated loudness spans −19.36 to −16.0 LUFS and true peaks span −4.80 to −1.99 dBTP. The warning was checked through the door filter and the plea through the pit reflection. [Six-line audition, hermit first](../Saved/VoiceReview/Hermit-and-Bandit.wav) |
| Unreal import | The editor loaded six non-looping SoundWaves and verified their durations against the mono 24 kHz masters. The import reported zero errors and warnings. |
| In-game mix and timing | The Vulkan Development run reached CP5 from the trailhead in 79.71 game seconds. Waveform matching identified all six performances, including each final syllable, in the recorded stereo game mix. The demand, response to the gold and parting order played in sequence. The order finished about 30 m behind the camera, nine seconds before the pack reached the bandit. [Cottage and roadside gameplay audio](../Saved/VoiceReview/In-game-dialogue.wav) |
| Linux Shipping | The cook reported zero errors and warnings. The archive contained updated data for all six dialogue assets. In the packaged game, normal keyboard and mouse input reached the demand, fatal shot and far-bank retry. Three fragments of the new demand matched the packaged stereo output with correlations of 0.91–0.97. The game exited normally. |

## Roadside bandit appearance, November 22, 2026

| Case | Observed result |
| --- | --- |
| Robbery and far-bank retry | The Linux Development clean driver reached CP5 in 79.38 game seconds. The recovery driver rushed the bandit, saw the fatal shot and returned to CP4. A failed gold diversion restored the pouch for the successful retry. |
| Character in motion | A Vulkan run followed the bandit through the demand, walk to the pouch, stoop and fall. Across 1,186 sampled frames the head, hat, hair and outfit hand joints followed their attachment points within 0.001 cm. The run reached the escape checkpoint. |
| Appearance in the clearing | The normal player camera shows the hat and crossed cartridge belts during the demand. Front, side and rear inspection views confirmed the raised collar meets the neck and the eyes and moustache remain visible beneath the brim. A leather strap edge lifts slightly at the far shoulder and hip in close-up. [Portrait](../Saved/BanditReview/Bandit-Portrait.png) · [Player camera](../Saved/BanditReview/Roadside-Encounter.png) · [Before and after](../Saved/BanditReview/Before-and-After.jpg) |
| Packaged Linux robbery | The Shipping cook and archive succeeded with zero cook warnings or errors. A muted 1440 × 900 session resumed an earned far-bank checkpoint in a private profile. Keyboard and mouse input brought Max to the bandit’s demand, crossed the firing boundary, displayed “You died” and returned Max to the far bank. The game closed normally. [Shipping recording](../Saved/BanditReview/Shipping-Robbery.mp4) |

## Cabin ground and doorway jump, November 22, 2026

| Case | Observed result |
| --- | --- |
| Flickering rectangles | The original Vulkan recording reproduced flat rectangles switching between forest litter and trail dirt in the cabin yard. The saved map had the spur, yard slabs and parts of the main trail occupying the same depth. A single stitched clearing now carries both ground textures. During the same 112-frame pan, 39 of 78 sampled locations flashed before the repair, and zero flashed afterward. [Matched recording](../Saved/CabinGroundReview/Cottage-Yard-Before-and-After.mp4) |
| Pack crossing the yard | All three visible wolves continued moving and posing through the original camera pass. Separate 60 Hz movement captures found that faraway wolves rose 128 cm when Max jumped, and that both flankers sometimes reversed direction on consecutive frames. On the revised ground queries, they stayed grounded through the same jump and those repeated reversals disappeared. |
| Open hollow | A private CP2 session used an open door and a lit firebrand. From each side, Max tried going straight and cutting around each jamb, first at walking speed and then sprinting. All 12 attempts fell into the hollow. Running jumps landed on solid ground on both sides. The raised lintel cleared Max’s head across a continuous 3.75 m range of tested entry takeoffs at normal speed. Downward traces found the tagged cover at floor level and the bottom 2.50 m below after digging. [Both jumps in Development](../Saved/CabinGroundReview/Cottage-Doorway-Jumps.mp4) |
| Cabin puzzle and checkpoint | The Development route took the shovel, dug from the side, knocked, watched the hermit step into the open hollow, jumped inside and barred the door. The fatal knock on intact boards and CP1 retry also passed. Both unlit departures triggered wolf capture and a CP2 restore. The lit departure cleared the pit and the clean run reached CP5 in 79.38 game seconds. |
| Packaged Linux game | BuildCookRun archived the Shipping game with exit code 0. The cook reported zero errors and warnings. UnrealPak confirmed the revised map, clearing mesh and material. The archived executable matched the compiled binary. A private copy of a previously earned CP2 save loaded successfully. Keyboard input took the burning brand at the hearth, opened the door and jumped outside. The inventory wheel still showed the equipped burning brand, and the game exited normally. [Shipping jump](../Saved/CabinGroundReview/Shipping-Doorway-Jump.png) |

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
