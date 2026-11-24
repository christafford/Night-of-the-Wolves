# Night of the Wolves

A single-player, third-person survival game set in a moonlit Pacific Northwest forest. The playable first leg begins on the trail and follows the pursuit through a hermit's cottage, a washed-out crossing and a roadside robbery. Beyond the bend, lightning brings down a fir across the logging road. Firelight leads into a cave.

## Play on Linux

Launch the Moonlit Trail Shipping build locally.

```sh
"$HOME/Documents/Unreal Projects/Night_of_the_Wolves/Saved/MoonlitTrail-Linux/Linux/Night_of_the_Wolves.sh"
```

A new trail opens from the lead wolf’s point of view. To watch it from the beginning while keeping your saved checkpoint, append `-NoTrailSave` to the launch command.

From the project folder, use `./Scripts/play_checkpoint.sh river` for the broken bridge, `./Scripts/play_checkpoint.sh bandit` to begin on the far bank just before the robbery, or `./Scripts/play_checkpoint.sh storm` to begin at the bend just before the lightning strike. Each preview uses a temporary copy of an earned checkpoint.

Run with **WASD**, look with the **mouse**, hold **Left Shift** to sprint, and press **Space** to jump. **E** interacts with the world and **F** uses the equipped item.

**Tab** opens the inventory wheel and pauses the game. Select **Bare hands** to put away a tool. Point at an item or use the arrow keys, then click or press **Enter** to equip it. Right-click, **Esc**, or **Tab** cancels.

**Esc** opens the pause menu. **R** retries from the latest checkpoint. The game also resumes a saved checkpoint on the next launch. Choose **N**, then **Enter**, to start a new trail from the beginning.

[Listen to “No Birds at This Hour”](Art/Source/Music/SW_MoonlitPursuit.wav) · [Watch the alpha take point in Linux Shipping](Saved/StormReview/Alpha-and-the-Pack.mp4)

<details>
<summary>First-leg playtest notes and footage (puzzle spoilers)</summary>

Select **Bare hands** before knocking. A confirmed switch away from the burning firebrand extinguishes it. At the broken bridge, climb the loading ramp to the logging trolley. Equip the cabin rope and press **F** to cast a loop over the high hook. Press **E** to ride. Once over the far landing, press **E** to let go. The trolley stops at the far end if you stay on.

[A failed sprint and a successful diversion](Saved/RobberyReview/No-Escape-Without-the-Pack.mp4) · [Lightning strike and cave](Saved/StormfallReview/Stormfall.mp4) · [Max in the river](Saved/StormReview/Max-in-the-Current.mp4) · [Five wolves in pursuit](Saved/FirstLegReview/Five_Wolf_Pursuit.mp4) · [Trailhead capture in Shipping](Saved/FirstLegReview/Wolves_at_the_Trailhead.mp4) · [The pack at the broken bridge](Saved/CreekScoreReview/Creek-Lookback.mp4) · [Wolf and hermit in the current build](Saved/CharacterArtReview/Wolf-and-Hermit.jpg) · [Earlier encounter footage](Saved/EncounterReview/Pack-and-Cabin.mp4) · [Max in the current build](Saved/MaxReview/Max.jpg) · [First-leg walkthrough with the earlier player model](Saved/FirstLegReview/Moonlit-Trail.mp4) · [Earlier six-view gallery](Saved/FirstLegReview/Moonlit-Trail.jpg)

</details>

[Build and editor workflow](AGENTS.md) · [Game design](docs/GAME_DESIGN.md) · [The five wolves](docs/WOLF_PACK.md) · [Forest authoring](docs/FOREST.md) · [Max and MetaHuman Creator](docs/MAX.md) · [Playtest results](docs/PLAYTEST.md)
