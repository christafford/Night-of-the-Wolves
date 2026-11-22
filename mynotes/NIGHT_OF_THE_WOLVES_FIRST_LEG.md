# Night of the Wolves

## First Leg: The Moonlit Trail

**Document type:** Level design and gameplay specification  
**Status:** Proposed design, ready to guide implementation and playtesting  
**Version:** 0.1 — November 22, 2026

This document develops the original five encounters into one playable opening. It preserves the forest chase, the hermit's cottage, the firebrand, the broken crossing, and the bandit. The detailed staging, safeguards, and timing values are proposed refinements; they are not claims about what the current project already implements.

## 1. Purpose and tone

The player is alone in a moonlit forest. Wolves are coming. There is no introduction, explanation, character biography, or friendly voice giving instructions. The player must keep moving, understand immediate problems, and use what they find to survive.

The protagonist is unnamed in the game. **Max** is an internal development name only.

The full game's objective is to survive until daylight. This first leg covers only the beginning of the night; reaching its end does not advance the clock to dawn.

The tone is tense, strange, and occasionally grimly funny. The cottage offers the appearance of safety, but its occupant is dangerous. Fire appears to solve the wolf problem, but only temporarily. The bandit thinks he has found a helpless victim and discovers that something worse is following them.

The opening should teach a small, reusable set of actions through play:

1. Run toward a readable destination.
2. Notice an environmental opportunity.
3. Collect and equip an item.
4. Use it at an appropriate place.
5. Give up a temporary advantage to make progress.
6. Turn an existing threat into an opportunity.

Use a successful, practiced run of roughly **two to four minutes** as an initial planning estimate. A first visit will take longer because of observation and retries. Measure the completed blockout before setting a duration target; do not lengthen the puzzles simply to fill time.

### Design priorities

- **Urgency with understandable causes.** Players should see or hear danger worsening before it reaches them.
- **Short, satisfying discoveries.** Each encounter has a central idea rather than a long chain of unrelated errands.
- **Fast recovery.** A failed rope swing should lead to another rope attempt, not a complete replay of the cottage.
- **Consistent item rules.** Equipment, consumption, and world interactions must behave predictably.
- **A continuous pursuit.** Obstacles and distractions delay the same pack; they do not make it disappear permanently.

## 2. Core gameplay rules

### 2.1 Movement and interaction

- Use a third-person camera that keeps the next useful piece of the environment visible.
- Provide movement, sprint, jump, interact, inventory wheel, and contextual item-use actions. Bindings should follow the project's input scheme.
- Sprint is unlimited in this first leg. A stamina meter would add another failure condition before the main puzzle language is established.
- Running along the intended escape route preserves or increases the player's lead during travel sections. Stopping, backtracking, or running toward the pack is dangerous.
- Running in circles does not create unlimited safety or reset an encounter.
- Keep required interactions brief. Avoid long inspection animations while a threat clock continues invisibly.
- Wrong-item attempts give a short response and preserve the item.

### 2.2 Infinite inventory, one equipped item

Inventory has **no capacity or weight limit**. Picking something up never requires discarding something else.

Only **one inventory item may be equipped at a time**. Everything else remains stored. Empty hands are also a valid selection.

Use a weapon-wheel-style radial selector:

- Show an icon, readable name, and relevant state for each item.
- Give the wheel a fixed, readable number of positions per page; add pages as the inventory grows. Unlimited storage must not produce hundreds of tiny wedges.
- Keep an empty-hands selection available.
- In this single-player opening, opening the wheel pauses the entire gameplay simulation, including wolves, water hazards, NPC behavior, and encounter clocks. The pause menu does the same.
- Previewing a selection changes nothing. Confirming it commits the equipment change; canceling leaves the current equipment and its state untouched.
- Do not allow movement, item use, or other world progress while the wheel is open.
- Disable equipment changes during a committed traversal animation, such as swinging, while retaining the ordinary pause function.

Ordinary pickups add items to inventory without changing equipment. An interaction that equips an item immediately must say so, such as **Take and equip firebrand**.

Opening a door, knocking, jumping, and gripping an already attached rope are contextual actions. They do not count as equipping a second inventory item. Animations can reposition the held object as needed.

There is **no item-combination or crafting system in this leg**. Using a rope on a branch is an environmental interaction. Picking up two supplies together does not combine them into a new item.

### 2.3 Firebrand behavior

The torch is a single persistent item made from a suitable burning piece of wood. It has two states: **Burning firebrand** and **Extinguished firebrand**.

- It deters nearby wolves only while burning and equipped.
- Confirming any switch away from a burning firebrand makes Max visibly extinguish it before storing it.
- The wheel previews this consequence: **Switching items will extinguish the firebrand.**
- Opening the wheel, highlighting another item, or canceling does not extinguish it.
- Re-equipping the extinguished item does not relight it.
- It can be relit at the cottage fireplace while that location remains accessible. Relighting is a contextual use, not crafting.
- Relighting does not restore the pack's earlier hesitation or the cottage door's condition.
- Picking up supplies, opening doors, and jumping do not extinguish an equipped firebrand.
- Water extinguishes it. Entering the creek's current is also fatal, so this does not create a swimming solution.

For the first version, omit a separate fuel meter. The meaningful limits are the wolves' diminishing hesitation and the decision to equip another item. The fireplace offers one suitable collectible firebrand; repeated interactions cannot create an inventory of spare burning logs.

### 2.4 Item loss and firearms

Use authored interaction targets for digging, attaching the rope, and throwing the gold. Unrestricted dropping and free throwing are outside this first leg's scope.

An item leaves inventory only when its valid use commits successfully. A canceled action, invalid target, or failed interaction must not consume it.

The hermit's shotgun and bandit's gun are threats, not collectible equipment in this opening. Their placement and outcomes must make that clear. Do not highlight them as loot or imply a shooting solution that the game does not support.

### 2.5 Pursuit and failure

Use encounter-specific pressure stages within the fiction of one continuous chase. Avoid one hidden countdown that kills the player regardless of doors, distance, or obstacles.

Each stage needs an understandable progression:

| Stage | What the player perceives |
| --- | --- |
| Approaching | Distant howls, moving shadows, brush disturbed behind Max. |
| Close | Individual growls, footfalls, visible eyes or silhouettes. |
| Imminent attack | A wolf advances, a door begins to fail, or the pack reaches the player's bank. |
| Failure | An actual attack, gunshot, or entry into the current causes death. |

Timers may drive this progression, but the visible threat must agree with the outcome. Never kill Max with wolves that are still visibly stranded across the creek.

Re-entering a trigger, reopening a door, switching equipment, or relighting the firebrand does not restart a spent allowance. Only genuine route progress or restoring a checkpoint establishes the next authored encounter state.

Use a short death presentation and prompt retry. Make the cause recognizable: wolves, shotgun, current, or a failed crossing. Do not require watching a long sequence again.

## 3. Route overview

| Encounter | Immediate problem | Solution | Lesson |
| --- | --- | --- | --- |
| Forest trail | Wolves are almost upon Max. | Run toward the cottage light. | Movement creates opportunity. |
| Cottage exterior | The apparent refuge has a violent occupant. | Use the shovel to prepare a pit, then knock. | Change the environment before triggering danger. |
| Cottage interior | A closed door buys only temporary protection. | Collect supplies, take a firebrand, and leave. | Inventory and temporary defenses. |
| Washed-out crossing | The creek blocks escape. | Extinguish the firebrand by equipping the rope, attach it, and swing across. | Progress can require sacrificing protection. |
| Bandit | An armed stranger blocks the path. | Throw the gold behind Max, drawing him toward the approaching pack. | Redirect the threat instead of fighting it. |

The trail remains the visual spine of the level. The cottage sits to its right; the trail continues to the creek and then to the bandit. Dense vegetation, steep banks, and fallen trees establish the traversable corridor naturally.

The cottage is a required supply stop. A player may initially run past it, but the bridge failure should be visible from a short distance ahead, before the player commits to a long, apparently valid bypass. The branch interaction can identify the need for rope without revealing the entire puzzle.

## 4. Encounter 1 — The forest trail

### Arrival and presentation

Start with Max facing down the moonlit trail. The path also extends behind him, but that direction contains the approaching wolves. The first composition should make the forward route immediately readable.

Give control immediately. Begin the **five-second opening response window only after the player can move**. Loading, fades, or an introductory camera movement must not consume it.

Use one brief movement/sprint prompt. A howl sounds behind Max, followed by closer movement in the brush. If he remains in place, the first wolf reaches him in about five seconds and the game restarts here.

### Successful play

Running forward creates separation. The cottage's illuminated window becomes visible through the trees before the turnoff.

Provide a visible reason that Max can soon stop to solve a puzzle: a narrow fallen-tree gap or similar obstacle that he passes quickly while the pack must clamber around it. Keep this traversal simple and readable. It should create time, not introduce a precision platforming challenge.

This gained lead must be sufficient to inspect the cottage approach, collect the shovel, and prepare the trap. The pack cannot remain five seconds behind Max while he performs twenty seconds of required actions.

### Outcomes

- **Success:** Max reaches the cottage approach with a usable lead.
- **Failure:** Standing still, retreating into the pack, or lingering on the trail leads to capture.
- **Checkpoint:** The initial spawn is the opening checkpoint. A second checkpoint is established at the cottage approach so learning the hermit puzzle does not repeatedly require the opening run.

## 5. Encounter 2 — The hermit's cottage

### Layout and clues

The cottage stands just to the right of the trail. It has one door and one small, glazed window. Firelight comes through the window. The shovel leans against the wall near the entrance.

The doorway sits above an **old drainage hollow covered by loose earth and rotten remnants of a plank cover**. Runoff has left a sunken patch just beyond the threshold. Exposed plank edges, a small dark gap, and different soil texture distinguish it from the surrounding ground.

This is the recommended refinement to the original hole: Max can clear and collapse an existing weak patch with a few shovel strokes, rather than excavating a person-sized pit from undisturbed ground in seconds.

Keep the resulting opening narrow enough to jump but awkward and deep enough to trap a person who falls into it. Give Max a stable place to stand beside the door while knocking; the prepared pit must not make the interaction unreachable.

The window offers a warning, not an alternate entrance. A moving silhouette, the outline of a long gun, and an angry mutter suggest that the occupant is dangerous. Keep the window too small to climb through. Window breaking is not a supported solution in this leg.

### Intended sequence

1. Notice and take the shovel.
2. Equip it using the inventory wheel.
3. Face the disturbed earth. The contextual action reads **Dig loose earth**.
4. Complete a brief digging animation that exposes the hollow.
5. Knock from the stable side of the doorway.
6. The hermit throws open the door, shouts, and rushes out along the entrance path.
7. He steps into the prepared pit and falls. His shotgun falls into the mud beneath him, out of reach.
8. Jump over the opening and enter the cottage.
9. Close and bar the door with one clear interaction.

The hermit remains alive, furious, and audibly stuck below the threshold. His predicament removes the immediate gun threat without requiring an additional combat system. Establish his inability to reach the dropped shotgun visually.

### Knocking before preparing the trap

If Max knocks before preparing the pit, the hermit bursts out, shoves Max backward if he is in the doorway, and fires. This is the intended fatal outcome of the unprepared approach.

Keep it brief and clearly attributable to the hermit. The warning clues must already exist before this failure; dying must not be the only way to learn that the occupant is hostile.

### Fairness and staging

- The dig prompt appears only at the supported patch. Do not suggest that any square meter of forest can become a trap.
- The door swing, hermit's rush, pit, and player's knocking position must be laid out together so the sequence works spatially.
- The hermit falls along his visible path; do not teleport him into the opening.
- The shovel action leaves the player enough time to knock, cross, and close the door when performed promptly.
- The prepared pit is a hazard to Max as well. A failed crossing can lead to a short failure and the nearby retry.
- Wolves can jump the narrow pit. The trap does not create a permanent wolf barrier.
- Once Max is inside, make the door handle and closing prompt easy to find with a third-person camera.

### Outcomes

- **Success:** Max is inside with the door closed and barred.
- **Failures:** Unprepared knock, capture outside, falling into the pit, or leaving the doorway open until wolves enter.
- **Checkpoint:** Save the secured-cottage state immediately after the door is barred, before collecting the indoor supplies.

## 6. Encounter 3 — Temporary refuge

### Room and atmosphere

The cottage is one compact, readable room. A fireplace with burning logs provides the light. A table holds a coil of rope and a partly open sack of gold coins. Arrange the table and fireplace so both are visible from the entrance.

Closing the door gives the opening its first short breath: the outside sound becomes muffled, Max's breathing is prominent, and the fire crackles. Then scratching begins. Impacts grow stronger. The door and bar visibly strain.

The cottage is temporary shelter. A closed door does not let the player remain here indefinitely. Use the door as the main escalating breach threat so there is one clear reason to leave.

### Supplies and mandatory-item handling

**Recommended change:** Give the table one deliberate **Take supplies** interaction that collects the rope and gold into **two separate inventory entries**. Show both pickup confirmations.

Both objects are mandatory for the intended route. Grouping their collection prevents a player from taking the rope, leaving apparently optional gold behind, and discovering an unsolvable problem after the creek.

This is not automatic looting: the player chooses to take the supplies. It is not crafting: the items remain independent. The table should visibly empty after collection.

The exit stays physically usable. If the player tries to leave before collecting supplies, give one short prompt drawing attention to the untouched table. Do not secretly add items or pretend the door is locked. A player who still leaves without rope cannot make the creek crossing and can restore the secured-cottage checkpoint.

### Taking the firebrand

One long piece of wood has a burning end and an unburned end suitable for gripping. Its interaction reads **Take and equip firebrand**. Max takes it by the cool end; avoid showing him casually gripping a fully burning log.

Collecting the supplies after taking the firebrand is safe because pickups do not replace equipped items.

Opening the door with the burning firebrand equipped produces immediate, readable feedback: nearby wolves recoil, growl, and keep their distance. They continue following when Max leaves.

The pit remains outside the doorway. Max jumps it again on departure, keeping the firebrand equipped. Provide a clear takeoff and landing, with the opened door and trapped hermit out of the jump path. The wolves' retreat from the flame makes room to land.

### Fire buys space

Initially the pack stays outside striking distance. As the player lingers, wolves begin feinting closer. A lead wolf tests the flame before the pack finally commits to an attack.

The player should infer, **“They are afraid of this, but I cannot stand here forever.”** Avoid a numerical courage meter in the normal presentation.

Opening and closing the door does not repair it. Returning to the fire does not restore the wolves' original fear. Any time remaining indoors must reflect what has already happened on that attempt.

### Intended departure state

| Item | State |
| --- | --- |
| Shovel | Stored; retained for future use. |
| Rope | Stored; ready to use at the creek. |
| Gold coins | Stored as one sack; not a currency counter yet. |
| Firebrand | Burning and equipped. |

### Outcomes

- **Success:** Max leaves with the supplies and burning firebrand, following the original direction of travel.
- **Failures:** Wolves enter through an open door, the damaged door gives way after prolonged delay, or the pack overcomes the fire deterrent outside.
- **Recovery:** Death restores the secured cottage, its full starting allowance, and the supplies in their original places. Max retains the shovel earned before that checkpoint.

## 7. Encounter 4 — The washed-out crossing

### Layout and geometry

The trail reaches a fast creek. The bridge has lost its central span; the far end is visible, but the gap is too wide to jump. Broken boards and debris in the current establish the danger.

Beside the bridge, a tree leans over the water. A strong limb extends **over the channel**, rather than running only parallel to the bank. Its anchor point and the far-bank landing shelf must support a believable diagonal swing.

Establish these four elements during the blockout:

1. A safe takeoff position on the near bank.
2. A usable anchor above the crossing route.
3. An unobstructed rope arc with clearance over the current.
4. A broad, moonlit landing shelf on the far bank.

The anchor must be reachable through the depicted action. Use one short contextual animation in which Max casts the rope over the limb and secures it. Do not show him tying a knot directly to an unreachable branch.

Test the actual swing path before dressing this location with scenery. Correct branch direction alone does not guarantee a valid crossing.

### Discovery and tradeoff

The far-bank shelf, overhead limb, and broken bridge should be visible in the same general view. The creek noise rises as Max approaches. Behind him, wolves edge closer to the flame.

The rope is the way across, but equipping it extinguishes the firebrand. Preview this consequence in the wheel, then show Max snuffing and storing the wood when the selection commits.

The wolves become bolder as the flame goes out. This is the opening's clearest expression of the one-equipped-item rule.

### Intended sequence

1. Reach the crossing with the burning firebrand equipped.
2. Identify the limb and landing shelf.
3. Select the rope. The firebrand becomes extinguished and remains in inventory.
4. Use the rope at the authored attachment point.
5. Complete the securing action. Only then does the rope leave inventory and become an attached world object.
6. Grab the attached rope from the takeoff position and swing.
7. Release while moving toward the landing shelf.
8. Land, regain ordinary movement, and continue up the path.

If securing is canceled or interrupted before completion, keep the rope in inventory. Once attachment completes, keep the world rope usable even if the player briefly steps away. Never leave an intermediate state with neither a stored nor an attached rope.

After commitment, the rope is not recoverable in this leg. It remains visible on the near-bank tree. The player finishes with the shovel, extinguished firebrand, and gold.

### Swing controls and failure

- Show a release prompt and a clear landing destination.
- Give the release a generous success window. Slightly early or late releases can trigger a ledge catch or stumble onto the same shelf.
- Releasing well outside the safe arc sends Max into the current and causes death.
- Holding indefinitely does not cross automatically. The swing returns toward the near bank and the player remains exposed to the approaching pack.
- Keep the camera readable and avoid foliage obscuring the landing.
- Use a consistent authored traversal for the first version. Unrestricted rope physics are unnecessary for establishing this puzzle.

### The wolves cross too

A successful landing buys a short lead. Show the pack reaching the water, hesitating, and then beginning to swim through a visible route downstream. Place that route so the wolves emerge behind Max along the continuing trail.

The creek delays them; it does not defeat them. If Max waits on the far bank, he sees the danger returning before it reaches him.

### Outcomes

- **Success:** Max lands safely and continues with gold still in inventory.
- **Failures:** Entering the current, missing the landing, or allowing wolves to reach him on either bank.
- **Checkpoints:** Save at the safe creek approach before the rope action, and again after a stable far-bank landing. A swing failure retries the crossing. A later bandit failure retries from the far bank.

## 8. Encounter 5 — The bandit

### Staging and motivation

Beyond the creek, the trail bends through rocks and thick brush. The bandit steps into the path and raises a gun.

> “Hands up. Don't move.”

Keep the creek within earshot. Its noise and the bend conceal the approaching wolves from him. Max has just seen them crossing, so the player knows something the bandit does not.

The bandit blocks the narrow route. A short warning and a visible change in aim communicate that trying to rush past him will provoke a shot. He also notices the player's bag or gestures toward their possessions, making robbery understandable without a dialogue menu.

Max adopts a brief compliance stance on entry; a separate hands-up button is unnecessary. Looking around, opening the wheel, and presenting gold are allowed. The shooting boundary concerns attempting to pass the bandit, not turning the camera to inspect the route behind Max.

### Revealing the gold

Equipping the sack counts as complying with the robbery. It must not trigger a shooting response merely because the player used the inventory wheel.

The bandit's eyes and gun track the gold. He becomes impatient:

> “The bag. Put it down. Slowly.”

Max can now make a contextual throw toward a clear patch behind him, near the bend. Show the destination before committing. This is a short, plausible toss, not a long throw through dense foliage.

The pouch lands where the bandit can see it, with a few coins spilling into the moonlight. The wolves remain concealed beyond the bend until he moves toward it.

### Intended sequence

1. Stop before crossing the bandit's shooting boundary.
2. Equip the sack of gold.
3. Notice his attention shift to it.
4. Aim the contextual throw behind Max, toward the incoming wolves.
5. Commit the throw. The gold leaves inventory and lands at the shown location.
6. The bandit orders Max to stay still, lowers his attention, and moves past him to retrieve the sack.
7. Max can immediately run through the opening while the bandit crouches to gather the spilled coins.
8. The pursuing wolves reach the bend and attack the distracted bandit.

The bandit goes for visible money because he believes Max is intimidated and the surroundings are safe. Avoid staging him as knowingly charging into a plainly visible wolf pack.

The attack follows the wolves' actual arrival. A fast player may throw the sack before they reach the bend; keep the bandit occupied gathering spilled coins during that short interval. Validate the route timings so his distraction remains believable. Do not teleport wolves to the gold or make the player wait for the attack before escaping.

### Attack and escape

The attack can be communicated through a startled shout, one panicked gunshot, and movement partly hidden by the bend. The bandit dies, as in the original idea. A prolonged close-up is unnecessary.

Keep control with the player. Do not spend the escape window on an unskippable cinematic.

The diversion begins when the bandit commits to the gold; the pack's attack extends it briefly. The attack area is immediately unsafe, so neither the gun nor coins become retrievable loot. There is no second robbery of the body.

After Max gains distance, a familiar howl sounds behind him. The pack is still coming. A recognizable lead wolf can reappear later to reinforce continuity.

### Failure cases

- Waiting until the pack arrives without creating the diversion leaves both Max and the bandit exposed; the encounter ends in failure.
- Trying to force a way past the aiming bandit provokes a fatal shot.
- Selecting the shovel or extinguished firebrand does not solve the confrontation.
- Canceling the gold throw or choosing an invalid target does not lose the pouch.
- Waiting beside the attack instead of escaping eventually leads to capture.

### Exit

The leg ends after Max runs beyond the next bend and establishes a lead. Commit the next checkpoint once Max has reached that safe segment and the bandit's attack has resolved, which can happen out of view. Do not hold Max at the bend while waiting for it. Never save while he stands beside the attack.

The player carries the shovel and extinguished firebrand. The rope remains at the creek and the gold remains behind. The next leg should give these retained objects new relevance.

## 9. Checkpoints and recoverable state

Checkpoints restore a deliberately playable encounter start. They do not save an arbitrary instant when death is already unavoidable.

The inventory column below describes the intended route. Save the actual owned items and their states; it is not a list of objects to grant on load. Never create an uncollected firebrand, relight one silently, or replace a consumed item without restoring its earlier world state too.

| ID | Location and trigger | Expected inventory and equipment | World state on restore |
| --- | --- | --- | --- |
| CP0 | Forest start. | Empty inventory; empty hands. | Max faces forward; opening five-second allowance starts after control returns. |
| CP1 | Cottage approach reached with the opening traversal complete. | Empty inventory; empty hands. | Shovel available, pit unprepared, hermit inside, door closed; a fair exterior-solving allowance. |
| CP2 | Cottage door closed and barred after trapping the hermit. | Shovel owned and equipped. | Max inside; hermit trapped; supplies and firebrand available; full interior starting allowance. |
| CP3 | Safe creek approach, before selecting or attaching the rope. | Shovel, rope, and gold stored; burning firebrand equipped. | Rope unattached; wolves behind at the authored starting separation; enough time for the crossing actions. |
| CP4 | Stable landing on the far-bank shelf. | Shovel, extinguished firebrand, and gold stored; empty hands. | Rope attached on the near bank; wolves starting their crossing delay; bandit encounter untouched. |
| CP5 | Escape bend beyond the resolved bandit encounter. | Shovel and extinguished firebrand stored; empty hands. | Gold spent, bandit dead, immediate escape complete; pursuit ready to continue into the next leg. |

For CP3, require the rope and gold before committing the crossing checkpoint. Preserve the actual equipment and firebrand state: choosing the rope a little early must not invalidate the checkpoint or relight the wood on restore. If either required supply is absent, retain CP2 as the recovery point.

For CP4, validate that gold is present before committing the irreversible-crossing checkpoint. Under the recommended grouped supply pickup and contextual-use rules, possession of the rope guarantees prior collection of the gold. Keep the validation as protection against later design changes or bugs.

If a player finds a viable way to skip a pickup that this leg does not strictly require, checkpoints preserve that omission. For example, a successful escape without taking the firebrand must not award one at CP4 or CP5. Later legs must guarantee access to anything they make mandatory, rather than assuming an item was collected merely because it appeared on the planned route.

On restore, reset inventory and world state together: pickups, attached rope, pouch, NPCs, doors, timers, wolf positions, equipment, and incomplete interactions. A consumed rope must not also reappear in the bag; a restored gold sack must not leave an extra sack on the ground.

Only a stable, living player can commit a checkpoint. Resolve overlapping death and success events once, so water and wolves cannot trigger multiple deaths or save an already dead character.

Establish each checkpoint once on forward progress. Saving it does not reset live wolf positions, door damage, or clocks. Restoring it deliberately creates the fair retry state; walking back through its trigger does not.

## 10. Starting values for pacing

All numbers below are **initial tuning suggestions**, not measured requirements. Apart from the requested opening five-second urgency, adjust them after playing the complete sequence with an unfamiliar player.

| Moment | Initial tuning target | What must fit inside the allowance |
| --- | --- | --- |
| Initial inaction | About 5 seconds. | Recognize the movement prompt and begin running. |
| Cottage exterior, from CP1 | About 30–40 seconds before capture if no progress is made. | Notice clues, take/equip shovel, dig, knock, cross, and close the door. |
| Digging | About 3–4 seconds. | Clear the weak cover; communicate the changed terrain. |
| Cottage interior, from CP2 | About 35–45 seconds before a visible door breach. | Observe the room, collect supplies, take the firebrand, and leave. |
| Cottage-to-creek travel | About 15–20 seconds at a direct run. | Follow a readable route and see the broken bridge ahead. |
| Fire deterrence outside | Enough for that run plus roughly 10–15 seconds of observation, then clear escalation. | Recognize the crossing problem before the pack commits. |
| Rope securing | About 2–3 seconds. | Show the cast and attachment without a prolonged animation. |
| Rope attachment through landing | Keep the prompt sequence within roughly 8–12 seconds. | Select, secure, grab, swing, and release. |
| Far-bank lead | Enough to reach the bandit plus roughly 15–20 seconds to solve the confrontation. | Hear the demand, recognize the gold reaction, and make the throw. |
| Bandit diversion and attack | Aim for about 8–12 seconds of useful escape time, consistent with the wolves' actual arrival. | Run through the opening and reach the escape bend. |

These allowances are related, not independent countdowns to start simultaneously. In particular, extinguishing the firebrand must still leave enough physical separation for the remaining rope actions when the player acts promptly. If that is impossible in the blockout, move the wolves farther back or shorten the action sequence.

Wheel and pause-menu time is excluded because the simulation is paused. Ordinary observation and interactions occur in active time.

## 11. Suggested implementation order

Use existing project architecture and naming where appropriate. The following steps describe behavior to add or verify; they do not prescribe new classes or require replacing working systems.

### Pass 1 — Shared rules and complete route

- [ ] Block out the trail, cottage, creek, and bandit bend as one connected route.
- [ ] Establish movement, interaction prompts, death, and checkpoint restoration.
- [ ] Implement unlimited inventory with a paged radial selector and one equipped item.
- [ ] Implement persistent item state and valid-use consumption rules.
- [ ] Establish one authoritative pursuit progression for each encounter.

### Pass 2 — Cottage loop

- [ ] Make the first five seconds playable and immediately retryable.
- [ ] Establish enough visible separation for the exterior puzzle.
- [ ] Implement shovel pickup, prepared pit, both hermit outcomes, and door closure.
- [ ] Implement grouped supply pickup, firebrand acquisition, and door deterioration.
- [ ] Verify the CP1 and CP2 restore states before adding visual polish.

### Pass 3 — Crossing

- [ ] Validate anchor, takeoff, swing arc, landing, and camera in the blockout.
- [ ] Implement the firebrand-to-rope switch and persistent extinguished state.
- [ ] Transfer the rope from inventory to the world only on successful attachment.
- [ ] Implement release timing, forgiving landing, current death, and CP3/CP4.
- [ ] Show the pack beginning its delayed water crossing.

### Pass 4 — Bandit and full-leg completion

- [ ] Establish demand, shooting boundary, and visible interest in the gold.
- [ ] Implement throw preview, committed consumption, diversion, and resumed pursuit.
- [ ] Implement CP5 and the final inventory state.
- [ ] Play the entire leg without developer shortcuts or injected items.

### Pass 5 — Clarity and atmosphere

- [ ] Add directional threat audio and readable escalation.
- [ ] Improve moonlight, firelight, silhouettes, animation, and interaction framing.
- [ ] Tune timing from real attempts, including players unfamiliar with the solution.

### Essential acceptance checks

- A player starting empty can complete the leg through ordinary actions.
- The environment suggests every required solution before a forced failure.
- Opening and canceling the wheel leaves the burning firebrand unchanged.
- Confirming a switch away extinguishes it, and re-equipping does not relight it.
- Door use and ordinary pickups preserve the selected item.
- Canceled or invalid item uses do not destroy or duplicate required objects.
- After a valid CP3 is committed, a swing failure retries there with the rope available again.
- A death after throwing gold retries at CP4 with exactly one gold sack restored.
- Checkpoint recovery cannot strand the player beyond the creek without gold.
- Door cycling, trigger re-entry, and equipment switching cannot refresh danger allowances.
- The same saved state produces a fair attempt without dependence on an NPC's leftover animation or position from the previous death.
- The intended full route finishes with only the shovel and extinguished firebrand retained from these four items; a successful pickup omission is preserved rather than silently repaired.

## 12. Optional additions and future directions

The mechanics above form the recommended first version. These additions can deepen it after the complete route works.

### High-value atmosphere

- **A recognizable lead wolf:** Give one silhouette or marking a distinctive appearance, then bring it back after each obstacle. It becomes a recurring presence without dialogue or exposition.
- **Unsettling cottage sounds:** Keep the trapped hermit's muffled anger audible beneath the wolf noises. The refuge never feels entirely comfortable.
- **A brief false sense of victory:** After the bandit diversion, let the immediate noise fall away for a moment. Then a familiar howl returns behind Max.
- **Coin sound as feedback:** Let the first jingle shift the bandit's gaze before he speaks, making his weakness visible before the player throws anything.

### Assistance options

- Offer a wider release window or an assisted rope release.
- Allow longer encounter allowances while preserving the same clues and outcomes.
- Provide captions or visual equivalents for directional danger cues so essential information does not depend only on hearing.
- After repeated failures, offer a short observation such as “The ground beside the doorstep is loose,” rather than immediately stating the full solution.

### Ideas for the next leg

Retained objects should matter again. The extinguished firebrand could be relit at a dangerous new fire source, creating a decision about whether to stop. The shovel could expose a culvert, remove loose earth from an escape opening, or lever an obstacle with a suitable edge.

Before adding new inventory objects, look for an interesting second use of what Max already carries. This gives the unlimited inventory a sense of continuity and makes remembering earlier discoveries valuable.

Item combinations can come later, once collection, equipment, contextual use, and item state are all clear. The first leg is complete without recipes, combat upgrades, currency shops, procedural terrain digging, or additional inventory restrictions.
