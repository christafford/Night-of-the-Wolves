# Night of the Wolves: game direction

## The experience

A third-person survival game set outdoors in a forest at night. The unnamed protagonist, provisionally called Max, has no backstory. He begins alone on a trail, visible in light from the full moon, with wolf howls and forest sounds providing ambience. A wolf pack pursues him throughout the game. The goal is to survive until daylight, running and solving environmental puzzles under pressure; wolf capture and other fatal outcomes return the player to the last checkpoint.

The user supplied [this visual reference](https://media.easy-peasy.ai/b535d61a-ed9a-48fb-90d0-329997e25ef5/4e7998ac-d2dd-4dd7-9bd7-4ffb57562189.png): realistic dark woodland, desaturated blue-gray moonlight, mist, tangled bare branches, mossy ground, and a weathered stone cabin. Use it as an atmospheric and visual-style reference; the established opening still has one cabin window, and incidental figures in the image are not game requirements.

## Opening encounter: intended events

- Max starts facing down the trail. If he fails to run, wolves reach him after five seconds; he dies and restarts at the same spot. Running down the trail keeps him ahead and brings him to a cabin.
- The cabin has a door, one window, and a shovel beside it. Approaching the door offers a knock prompt using the appropriate action binding. An unprepared knock brings out an angry, drunk hermit, who grunts in surprise and shoots Max with a shotgun. Max dies and restarts. Waiting five seconds at the cabin also lets the wolves catch him.
- Approaching the shovel offers a take prompt. Taking it equips it by default. With the shovel, Max can dig a hole a little way from the door before the wolves arrive. He must approach the door without falling in. After he knocks, the hermit runs out to shoot him but falls into the hole. Max can enter the cabin and shut the door to keep the wolves away for a while.
- Every wolf capture uses the same quick sequence: a scary red-eyed wolf lunges toward the camera with “You died” over the screen. The hermit's grunt and shotgun distinguish that death.

The cabin is temporary shelter, not the game's ultimate victory. Further encounters are to be developed later.

## Open decisions and earlier proposals

- The exact pursuit timing during pickup, digging, moving around the hole, knocking, the hermit's response, and entering and closing the door is not settled. The complete solution must be feasible and fair; do not silently suspend or reset pursuit during interactions.
- The consequence of Max falling into the hole is unspecified. A prepared pit with removable ground and collision was suggested as an implementation, not mandated.
- A checkpoint upon trapping the hermit, entering the cabin, and closing the door was proposed but not confirmed. Likewise, the earlier discussion proposed that an open door or closing it from outside does not provide shelter.
- The window's role is unspecified; it may initially be non-interactive. Controls should be configurable and prompts reflect the active binding; “A” and “X” were examples.
- A progress-based pursuit model was proposed for initial prototyping: stopping or failing to make escape progress lets wolves close in, so running against an obstacle does not grant permanent safety. Visible pack behavior can be developed later.

## M3 prototype decisions to playtest

Taking and digging are immediate placeholder actions; the wolves keep moving during them and during the hermit's reaction. Max can fall into the physical pit, where pursuit still applies; no separate fall-death rule is implemented. After the hermit falls, closing the door from inside provides temporary shelter and ends pursuit for this opening prototype. It does not establish a persistent checkpoint or complete the full game. These are provisional choices, not settled pacing or checkpoint rules.

Following the user's playtest, picking up the shovel grants ten additional seconds of wolf lead once per attempt. The wolves continue pursuing, and the extra allowance counts down even while Max moves. This is a temporary pacing adjustment.
