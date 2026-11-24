# The five wolves

The game mode’s HuntingPack component owns five AI-controlled `AHuntingWolf` pawns shared by the cottage, crossing and capture camera. Passing a checkpoint keeps the pursuit live. Retrying places the same five at the restored encounter, clears knowledge, paths, reservations and gait state, and restores Max’s supplies and puzzle state.

| Wolf | What to watch for |
| --- | --- |
| Leader | Watches the trail from a shoulder behind the direct pursuer. A short growl and a head turn toward the newly assigned wolf’s approach can signal a change. An available packmate can signal when the leader is blocked. |
| Pressure beta | Follows the shortest useful approach behind Max and usually holds the attack reservation. |
| Flanker beta | Checks a reachable rear quarter in open ground. Often takes direct pressure when the beta gets stuck. |
| Interceptor scout | Takes a separate route around an obstruction when it can reach the predicted quarter in time. A turn, slowdown or loss of sight can invalidate the intercept. |
| Tracker scout | Checks the observed approach. When the pack loses sight, investigates the last observation, then advances between search locations. |

Build, coat shading and stride phase distinguish the five on the shared timber wolf skeleton. The tracker lowers its nose. The wolf that earns the capture performs the pounce. Signals use the existing growl and procedural head poses. Individual calls and dedicated tracking or signal clips remain presentation work.

## Opening and capture

On a fresh trail the camera runs at the silver leader’s eye height. Four packmates gallop ahead around the bends. On the final approach, the camera rises behind the group while the leader passes them and takes point. The five-second idle lead starts when the view reaches Max and control returns. Retrying CP0 goes directly to Max.

After a pounce, the static wolf portrait appears with a separate close snarl. Retrying stops any remaining sound from the card. The snarl uses an excerpt from a recorded canine attack. [Recording and edit](../Art/Source/Presentation/SOURCES.md)

## Perception and movement

Sight requires range, a clear collision trace and a suitable facing at distance. Successive sightings estimate velocity. After losing sight, wolves can hear a coarse footfall location according to step strength and obstructions. Only a sighting can authorize a prediction or attack. After three reached search stations, a wolf advances its sweep from its own feet using terrain and the last observed heading. The tracker returns to the last observation after seven arrivals. Both counts are editable under **Search**.

The surface planner checks footing, slope and body clearance along complete routes. A scout’s alternate must take a distinct branch and meet a travel-time estimate. Beyond the creek landing, boulders separate the main track from an outer shoulder. Narrow ground calls for a staggered file. Wolves reserve destinations and check separation as they move.

The game mode owns the opening deadline, firebrand protection and encounter timing. Wolf capture requires a shared commitment and a nearby wolf with clear access. The door, breach, broken bridge, swim and bandit diversion have authored actions that release hunting assignments and age the pack’s knowledge. During Max’s trolley ride the wolves check the bank, then swim downstream in the creek shot.

## On the far road

The bandit follows coins that land in the approaching pack’s path. A wolf must reach him and strike before Max can earn the escape-bend checkpoint. The pack stays with the fallen robber for the escape window, timed from the return to Max. A misplaced throw leaves the pack hunting Max and the robber covering him with his gun. Trying to run past the armed robber or around his flank also draws a shot. A retry restores the far-bank encounter and its single sack.

At the lightning flash the five wolves stop hunting Max. They crouch, pin their ears and turn toward the tree. When the burning timber hits the ground, they wheel away and gallop downhill. Each starts at its actual position, including scouts still near the creek. A wolf checks the ground and goes around rocks to reach cover. Once there, the pack keeps its distance for as long as Max spends at the fire or in the cave.

## Tuning and inspection

Select the **HuntingPack** component and expand **Wolf pack**. The native defaults are in [WolfPackComponent.h](../Source/Night_of_the_Wolves/WolfPackComponent.h) and [WolfPackTactics.h](../Source/Night_of_the_Wolves/WolfPackTactics.h).

| Setting | Default |
| --- | --- |
| Sight, base ground speed, pounce reach | 23.5 m, 6.45 m/s, 2.8 m |
| Attack slots, support standoff, destination spacing, flank width | 1, 1.72 m, 1.12 m, 2.45 m |
| Check sight, plan one wolf, rotating through the five | Every 0.045 s, every 0.075 s |
| Fresh sight, investigation memory, prediction horizon | 0.65 s, 3.6 s, 2.4 s |
| Distance advantage for assigning direct pressure | Beta 4.10 m, flanker 2.85 m |
| Ordinary commitment, retry after failed specialty | 0.70 s, 1.65 s |

In a Development game or PIE, enter `wolf.PackDebug 1` in the console. Each row names the wolf, current assignment, path status and waypoint, destination, knowledge source and age, attack permission and last decision reason. Lines show the remaining paths. Magenta marks Max, yellow marks the shared belief and orange marks an active interception prediction. Enter `wolf.PackDebug 0` to hide it.

The [pack component](../Source/Night_of_the_Wolves/WolfPackComponent.cpp) moves the pawns, checks perception and validates a pounce. The [tactical coordinator](../Source/Night_of_the_Wolves/WolfPackTactics.cpp) assigns roles and reserves approaches. The [navigation service](../Source/Night_of_the_Wolves/WolfPackNavigation.cpp) checks complete routes against the playable surfaces. Each pawn’s [mesh component](../Source/Night_of_the_Wolves/WolfMeshComponent.cpp) carries its coat, stride and head gestures.

## Repeatable checks

Run the fixed-step checks from the project root after building the Development Editor. Each fixture exits with status 0 on success.

Use `Wolf.TestOpening` on a fresh trail to check the four visible runners, the fifth overtaking them, all five in frame, the five-second lead and the first retry. To verify the actual snarl in the mixer, run with sound and `-OpeningPackAudioTest`. Add `-OpeningPackPhotos` with rendering enabled to save the two camera views and the death card under `Saved/StormReview`.

```sh
WOLF_EDITOR="$HOME/UnrealEngine-5.9-preview-2/Engine/Binaries/Linux/UnrealEditor"
WOLF_PROJECT="$PWD/Night_of_the_Wolves.uproject"
for case in TestPackNavigation TestPackTactics TestPack TestRoadBranch
do
  "$WOLF_EDITOR" "$WOLF_PROJECT" /Game/NightOfTheWolves/Maps/M1_Trail \
    -game -unattended -nop4 -nullrhi -nosound -NoTrailSave -UseFixedTimeStep -FPS=60 \
    "-ExecCmds=t.MaxFPS 0,Wolf.$case" "-abslog=/tmp/wolves-$case.log" || break
done
```

`TestPackNavigation` checks complete routes, branch separation, a newly blocked corridor and missing ground. `TestPackTactics` advances five sets of feet along collision-checked routes for three simulated minutes with controlled sense input and blocked reports. It then hides the quarry for a full minute. The observed search reached 52.8 m from the stationary belief and the tracker returned to it seven times. `TestPack` exercises the five possessed pawns with actual sightlines, movement, a reversing target, capture and repeated retries. `TestRoadBranch` runs the scout along the outside shoulder on the playable far road and reverses Max to check cancellation. The first-leg drivers exercise the ordinary trail, cottage and crossing. Results and rendered evidence are in [the playtest record](PLAYTEST.md).
