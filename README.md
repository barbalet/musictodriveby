# musictodriveby

## Description

MusicToDriveBy is a first and third person perspective suburban environment generator allowing player to move from first person perspective to third person perspective while being on foot, on bicycle, on motorcycle, moped or car. The game also allows for melee weapon use including rocks, knives, machetes, pistols, revolvers, rifles, sawn off shot guns, shot guns, AK47s and M16s with the related caliber bullets, shotgun shells and rifle cartridges and clips.

The environment is  South Central Los Angeles with the gangs identified:

https://www.google.com/maps/d/u/0/viewer?mid=1ul5yqMj7_JgM5xpfOn5gtlO-bTk&ll=33.98203848816218%2C-118.45161449214874&z=9

The game is aesthetically similar to the Grand Theft Auto series of first and third person perspective games.

The game is written in Metal with SwiftUI and a C engine core.

Please plan for the cycles needed to create this game allowing for 100 cycles or so until completion.

### Current Cycle 

35

Cycle 34 was completed on April 17, 2026.
See `docs/cycles/cycle-34.md` for the cycle report.

### Cycle Plan

Assume each cycle ends with:

- a playable build
- short notes on what changed
- a short list of blockers and risks
- a locked goal for the next cycle

The first goal is not full content coverage. The first goal is a playable vertical slice that proves the SwiftUI + Metal + C engine stack can support traversal, vehicles, combat and basic world simulation.

**Pre-cycle setup (this cycle)**

- Confirm the primary development target, input devices and performance budget for early development.
- Define module boundaries between SwiftUI app shell, Metal renderer and C engine core.
- Agree the minimum scope for the first vertical slice so Cycle 0 starts with a narrow target.
- Set up issue tracking, a simple definition of done, and a repeatable build/run checklist.

**Cycle 0: Project bootstrap and first playable block**

- Create the app shell and wire SwiftUI, Metal and the C engine core together.
- Stand up input handling, timing, update loop, camera control and a single graybox neighborhood block.
- Establish coordinate system, asset conventions, debug overlays and profiling hooks.
- Exit criteria: the app launches, the player can walk a simple block in first person, and the work for Cycles 1-4 is ready to pull.

**Cycles 1-4: Core on-foot traversal**

- Add walking, sprinting, basic collision, curb and sidewalk handling, and camera smoothing.
- Prototype first person and third person camera modes with a clean toggle between them.
- Build a reusable graybox street segment with roads, sidewalks, lots and basic props.
- Deliver a stable traversal loop that feels credible before more systems are layered on top.

**Cycles 5-9: World layout and streaming foundations**

- Define block, district and streaming chunk formats for the South Central Los Angeles inspired world.
- Build the first road graph, lot generation rules and navigation surfaces.
- Prototype world streaming, culling and spawn/despawn rules for props, NPCs and vehicles.
- Deliver a connected set of neighborhood blocks that can stream in and out without obvious breaks.

**Cycles 10-14: Vehicle fundamentals**

- Implement entering and exiting bicycle, motorcycle, moped and car states.
- Build placeholder handling, acceleration, braking, turning and collision for each vehicle class.
- Add first person and third person vehicle cameras and transitions from on-foot play.
- Deliver a loop where the player can move between walking and driving without breaking the simulation.

**Cycles 15-19: Combat foundations**

- Implement melee weapons, hit detection, damage reactions and simple inventory slots.
- Add firearms, reload states, ammunition handling, projectile simulation and impact feedback.
- Support pickups, weapon switching, basic aiming and readable combat UI feedback.
- Deliver a small combat sandbox with placeholder enemies and a stable damage model.

**Cycles 20-24: NPC population and systemic response**

- Add pedestrian spawning, ambient movement, simple schedules and reaction states.
- Add traffic vehicles, lane following, stop/start logic and basic avoidance.
- Implement simple hostile, flee and investigate behaviors that react to the player.
- Milestone: end this phase with the first playable vertical slice combining traversal, vehicles, combat and NPC response.

**Cycles 25-29: Gang territories and encounter rules**

- Define factions, territories, spawn logic and relationship rules.
- Add patrols, ambient gang presence and escalation when the player crosses boundaries.
- Build encounter templates for intimidation, chase, melee and ranged conflict.
- Deliver a map that begins to feel socially structured rather than mechanically empty.

**Cycles 30-34: Mission framework**

- Build objective, trigger, checkpoint, success and failure state systems.
- Support mission scripting hooks for dialogue, captions, pickups, drop-offs and combat beats.
- Add simple persistence for mission progress during a session.
- Deliver a small set of repeatable mission templates that can carry the rest of production.

**Cycles 35-39: World content pass 1**

- Expand streets, alleys, storefronts, homes, parking lots, parks and landmark corners in graybox form.
- Add civilian and gang population variety, vehicle variety and environmental storytelling props.
- Improve navigation readability and line-of-sight design for traversal and combat.
- Deliver a broader playable district with recognizable sub-areas.

**Cycles 40-44: Character feel and animation pass**

- Add placeholder animation sets for on-foot movement, vehicle mounting, combat and reactions.
- Improve recoil, impacts, camera shake, death states and hit readability.
- Refine third person presentation so the player character is readable and responsive.
- Deliver a build where action feedback starts to feel intentional rather than purely technical.

**Cycles 45-49: Internal alpha integration**

- Connect traversal, vehicles, combat, NPCs, factions and missions into one stable loop.
- Cut or defer weak features so the alpha represents the intended game instead of an overloaded prototype.
- Add save/load foundations for position, inventory, mission state and world state where feasible.
- Milestone: end this phase with an internal alpha that can be played start to finish in a limited slice.

**Cycles 50-54: World content pass 2**

- Extend map coverage, landmark density and district identity.
- Add selected interiors or interaction hotspots only where they strengthen the main loop.
- Improve signage, street clutter, lighting placeholders and traversal landmarks.
- Deliver a world that supports longer sessions without obvious content cliffs.

**Cycles 55-59: Audio and atmosphere**

- Add ambient city sound, pedestrian chatter hooks, weapon sounds, vehicle sounds and music systems.
- Prototype radio or dynamic music choices that fit the driving fantasy.
- Mix audio for readability so vehicles, gunfire, footsteps and UI cues are easy to parse.
- Deliver a build where the atmosphere materially improves immersion.

**Cycles 60-64: Progression and economy**

- Add money, ammunition economy, pickups, safe locations or equivalent progression anchors.
- Define unlocks or gated access for vehicles, weapons, districts or mission chains.
- Persist player state in a way that rewards continued play without overcomplicating early systems.
- Deliver a core progression loop that gives the sandbox longer-term structure.

**Cycles 65-69: Pressure and consequence systems**

- Add law enforcement, witnesses or other systemic opposition as appropriate for the design.
- Implement heat escalation, search behavior, escape states and cooldown rules.
- Balance the consequences of reckless play against the freedom fantasy.
- Deliver a world that can push back on the player in a readable and dramatic way.

**Cycles 70-74: HUD, menus and accessibility**

- Build HUD, minimap or directional guidance, prompts, pause flow and settings menus.
- Add subtitles, remapping, camera options and motion/readability settings where practical.
- Refine onboarding so first-time players can learn traversal, vehicles and combat quickly.
- Milestone: end this phase with a beta-quality build and a locked feature scope.

**Cycles 75-79: Optimization and technical hardening**

- Profile CPU, GPU, memory and streaming performance across representative gameplay scenarios.
- Optimize AI, rendering, animation and simulation hotspots that threaten the performance budget.
- Improve loading, hitching, crash handling and debug telemetry.
- Deliver a build that behaves consistently enough for heavier QA and playtesting.

**Cycles 80-84: Content polish and mission pass**

- Improve mission pacing, encounter variety, district identity and standout moments.
- Rebalance weapons, vehicles, enemy pressure and rewards.
- Remove filler content and strengthen the parts of the game that are already fun.
- Deliver a more focused game rather than a merely larger one.

**Cycles 85-89: QA and bug burn-down**

- Run regression passes on movement, combat, vehicles, saves, streaming and mission flow.
- Fix collision holes, spawn bugs, deadlocks, save corruption risks and long-session stability issues.
- Use playtest feedback to prioritize only the bugs and rough edges that damage the core experience.
- Deliver a build that is stable enough to call a serious release candidate effort.

**Cycles 90-94: Release candidate preparation**

- Lock content except for critical fixes and final balancing.
- Finalize credits, licenses, packaging and release documentation.
- Capture screenshots, footage and supporting release material as needed.
- Milestone: end this phase with a release candidate build.

**Cycles 95-99: Final polish and ship decision**

- Fix only release-blocking bugs and performance failures.
- Verify install, launch, save/load and long-session behavior on target hardware.
- Make the final ship/no-ship call based on stability and the strength of the core fantasy.
- If needed, use these cycles as controlled contingency rather than as an excuse to add scope.

**Milestone gates**

- End of Cycle 24: first playable vertical slice
- End of Cycle 49: internal alpha
- End of Cycle 74: beta with locked feature scope
- End of Cycle 94: release candidate
- End of Cycle 99: ship target or disciplined extension decision

### Build And Run

- `swift build`
- `swift run MusicToDriveBy`
- Open `MusicToDriveBy.xcodeproj` in Xcode and run the `MusicToDriveBy` scheme on `My Mac`
- Click inside the game view to capture mouse look, and press `Esc` to release it again.
- Walk north along the starting sidewalk, press `T` near the glowing lead pipe or pistol pickup, and confirm the in-game HUD appears over the Metal view with health, weapon, encounter, street, and prompt panels instead of leaving combat readability entirely to the floating debug window.
- Stay in the lane long enough for the lookout to pressure you and confirm it now slides between distinct firing angles instead of pacing in one strip. The encounter panel should call out the current angle and show reacquire timing when it is shifting.
- With the pistol equipped, step behind the low cover, the west post, and the new east barrier in turn, and confirm the lookout changes angle to break static cover while blocked shots still splash against the first solid prop in the lane.
- Fire the pistol or land a close melee hit near the witness and confirm the bystander now shifts into investigate or flee behavior while the new street card reports whether the lane is calm, escalating, or cooling.
- Break line of sight behind cover or escape in a staged vehicle after the lookout is active, then confirm the world now shows both the hostile search marker and the broader street-incident marker instead of jumping straight from pressure to idle.
- Stay off the street edge long enough for the search to expire and confirm the lookout now settles through a brief reacquire window before firing again, while the street card reports that the block is still holding, cooling, or actively normalizing instead of snapping straight back to calm.
- Push the incident a bit harder and confirm a second authored bystander on the far side of the block now joins the reaction instead of leaving the street response to only one civilian.
- Watch the nearby sidewalks once the incident is live and confirm some ambient pedestrians now peel away from the block with warmer reaction cues instead of marching straight through the danger pocket.
- Cross near the lane after civilians have reacted and confirm ambient traffic now eases back from the block and the nearest adjacent road link while nearby block and link pulses warm up with the spillover instead of isolating all pressure to one strip of street.
- Approach the lane mouth slowly before the fight and confirm the street card or prompt now calls out a readable territory line instead of treating the whole block as socially neutral from the first step.
- Approach the lane mouth again on foot and confirm a visible court-set sidewalk runner is idling outside the line instead of leaving the edge socially empty.
- Step a little closer without fully committing to the lane and confirm the runner now screens the entry first, then briefly clamps it with the inner post from complementary angles if you hesitate at the boundary. The street card or prompt should call out that clamp instead of only a generic watched sidewalk.
- Roll a staged vehicle along that same edge, let the runner clear out, then back off toward the lane mouth and confirm the vehicle line reforms with a different rhythm from the on-foot retake.
- Step across the line and confirm the runner no longer holds the same screening posture once the inner post or lookout owns the pressure. The runner should clear the lane mouth instead of continuing to block the same angle.
- Back off to the boundary after that clear-line beat and confirm the runner now reforms the line instead of snapping straight from clear to idle. The street card or prompt should switch into reform language during that retake.
- Step back in before that reform settles and confirm the runner now hardens the line again instead of jumping straight to the old generic clamp. The street card or prompt should switch from reform into hardening as the edge locks back up.
- Once that harden beat is live, hover at the lane mouth and confirm a short commit window now appears instead of leaving the push as pure ambient pressure. The marker, prompt, and encounter card should all acknowledge the same line-break beat.
- Push through the hardened edge on foot and confirm the objective flips from commit window into a live hold state rather than opening a separate mission panel or checklist.
- Stay inside the pocket long enough for that push to land and confirm the line now resolves into a short completed state with the HUD and street card both acknowledging that the hardened edge has been broken.
- Back out too early during that beat and confirm the objective collapses cleanly back into the existing runner/post territory behavior instead of hanging around after the line has settled.
- Push farther into claimed turf and confirm the sidewalk runner now clears the line while the inner post owns the pocket, with the encounter readout shifting from clamp or screen language into clear/post pressure once the handoff is live.
- Watch the inner post during that retake and confirm it now has a clearer completion rhythm: farther back it should relax more, while a closer recommit should make it step up harder instead of feeling equally strong at every distance.
- Back off once the lookout search and civilian panic have ended, and confirm the sidewalk runner settles first while the inner post lingers deeper for a beat before the pair fully cools back toward idle.
- Repeat the approach from a slightly different lateral angle or after a hot return and confirm the runner and inner post do not frame the same line the same way every time. The edge screen, clamp, clear-line beat, reform, harden, and deeper support pocket should feel like a small staged setup instead of one rigid placement.
- Stay with the block after the normalization pulse fades and confirm the street now passes through a short reopening phase instead of dropping straight to idle. The street card should call out reopening while nearby traffic retakes the lane and adjacent link in a softer, cooler handoff.
- Provoke the lane, pull out of the territory, then re-enter before it cools and confirm the block is now received as hot territory in a readable paired sequence: the runner should recover first, the inner post should take the handoff quickly, the street card should call out a burned block, and the hostile should pick you up faster instead of replaying the same cold start.
- Treat the whole pass as the first paired-fallback check: sidewalk runner, inner post, on-foot line skim, vehicle line skim, deeper push, runner-to-post handoff, staggered fallback, hot re-entry, and the existing witness/traffic spillover should now read as one socially occupied slice instead of one lookout plus labels and markers.
- Fire until the clip runs low, press `Y` to reload, and confirm the prompt pill, health card, weapon card, encounter card, and street card stay readable while hostile movement, multi-civilian reaction, traffic reaction, and staged vehicle handoff prompts overlap.
- Treat this whole pass as the first post-milestone faction check: traversal, staged vehicle escape, lane combat, civilian spillover, traffic easing, normalization, reopening, territory crossing, and hot re-entry should now read as one socially structured graybox slice instead of a neutral sandbox.

The current prototype is a macOS graybox four-block neighborhood slice with two authored streaming chunks, frontage-template-driven block variants, hotspot hooks, engine-exported traffic occupancy hooks, route-aware placeholder traffic with stop-zone holds and early near-miss hazard pulses, staged sedan, coupe, moped, bicycle, and motorcycle handoffs with ranked selection cues and lock controls, and an expanding combat scaffold that now includes a lead-pipe pickup, a pistol pickup, weapon-slot switching, ammo and reload state, muzzle and impact feedback, cover-aware firearm blocking, a lookout hostile that can return fire, pressure-anchor-driven hostile repositioning, two authored civilian reaction sources around the lane, ambient pedestrians that can peel away from live incidents, a lightweight hostile last-seen search state, a persistent street-incident layer that spills into the nearest adjacent road links, a distinct street-reopening handoff after normalization, warmer spillover and cooler normalization/reopening ambient-link feedback, and the first faction-territory scaffolding with a readable court-set boundary at the lane mouth, claimed-turf readouts inside the block, a visible court-set sidewalk runner, a second ambient inner post replacing the old deep marker, simple runner-to-post handoff rules, a clearer sidewalk screening beat before full territory commitment, a brief complementary clamp at the boundary when the player hesitates, a lightweight clear-line beat once the inner post or lookout fully owns pressure, a lightweight reform/re-approach beat when the player backs out, a lightweight harden/brace beat when the player recommits during reform, a lightweight lane-mouth commit window once that hardened line is live, a short push-through objective progress and completion beat without a separate mission panel, clearer inner-post completion rhythm based on boundary distance, staggered paired fallback after heat and search drop, a slightly looser set of runner and inner-post stage pockets so repeated approaches do not silhouette the same way every time, different on-foot versus vehicle territory reception, adjoining-sidewalk pressure before the full lane entry, and a hot re-entry escalation rule that makes the lookout receive you faster after you leave the block burning. The in-scene combat HUD now carries that territory state alongside the street/system readout, so player health, recovery, systemic pressure, faction presence, sidewalk watch, entry screen, entry clamp, clear-line handoff, reform, hardening, commit window, line break, territory crossing, and hostile escalation can be read together instead of as unrelated debug fragments. Use the on-screen overlay and the floating controls window together for controls, chunk visibility, staged vehicle prompts, combat state, street pressure, traffic reaction, civilian reaction, territory heat, watch, patrol alert, inner-post alert, handoff ranking and lock state, grip, lane, bump, recovery, hazard feedback, and active block metadata.

### Any questions ###

Contact barbalet at gmail dot com
