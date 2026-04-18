# musictodriveby

## Description

MusicToDriveBy is a first and third person perspective open-world Los Angeles environment generator allowing player to move from first person perspective to third person perspective while being on foot, on bicycle, on motorcycle, moped or car. The game also allows for melee weapon use including rocks, knives, machetes, pistols, revolvers, rifles, sawn off shot guns, shot guns, AK47s and M16s with the related caliber bullets, shotgun shells and rifle cartridges and clips.

The environment is the full Los Angeles gang-map footprint identified here:

https://www.google.com/maps/d/u/0/viewer?mid=1ul5yqMj7_JgM5xpfOn5gtlO-bTk&ll=33.98203848816218%2C-118.45161449214874&z=9

The game is aesthetically similar to the Grand Theft Auto series of first and third person perspective games.

The game is written in Metal with SwiftUI and a C engine core.

Please plan for the cycles needed to create this game allowing for 125 cycles or so until completion.

### Current Cycle 

56

Cycle 55 was completed on April 18, 2026.
See `docs/cycles/cycle-55.md` for the cycle report.

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

**Post-alpha direction reset**

- The internal alpha proved the core loop, but the next 50 cycles must pivot hard toward production quality.
- The linked Los Angeles gang map becomes a reference layer for territory layout, district identity and neighborhood coverage instead of a loose inspiration.
- The world must expand from a graybox slice into many miles of recognizable Los Angeles roads, intersections, alleys, freeways, frontage and district transitions that can be driven and walked in-app.
- Blocky geometry and placeholder-looking scenery are no longer acceptable; the target is a professional game presentation with realistic materials, lighting, props, vehicles, streets and skyline composition.

**Cycles 50-54: Los Angeles reference ingestion and geo pipeline**

- Lock the new product target around realism, coverage and district authenticity rather than around placeholder breadth.
- Build a production data pipeline that can ingest real Los Angeles road hierarchy, parcel structure, freeway corridors, major intersections and district metadata at city scale.
- Use the provided gang-map reference as a design layer for territory placement, faction boundaries, neighborhood emphasis and mission geography.
- Define streaming tiles, road kits, block templates and art budgets around many-mile traversal instead of around small graybox chunks.
- Deliver a build that can display a believable multi-mile Los Angeles road network skeleton with named districts and map-aligned faction zones.

**Cycles 55-59: Production road network and street reconstruction**

- Replace placeholder street strips with production road generation supporting lane counts, medians, curbs, gutters, sidewalks, crosswalks, alleys, driveways and realistic intersection geometry.
- Add support for traffic signals, signage anchors, lane markings, utility corridors, retaining walls, fences and freeway edge conditions.
- Rework navigation, vehicle handling surfaces and collision around detailed road geometry rather than around broad block primitives.
- Establish art and design rules for South Los Angeles residential grids, commercial corridors, industrial edges and freeway-adjacent spaces.
- Milestone: end this phase with a driveable and walkable many-mile road pass that no longer reads as blockout geometry.

**Cycles 60-64: Terrain, parcels and district-scale world assembly**

- Build parcel subdivision, lot grading, curb elevation, setback rules, parking layouts and backyard or alley conditions that fit Los Angeles neighborhood patterns.
- Add support for drainage slopes, embankments, bridges, underpasses, overpasses and rail or river corridor treatment where needed for believable city topology.
- Assemble multiple contiguous districts from the real road foundation so neighborhood transitions feel geographically grounded rather than stitched together.
- Replace repeated placeholder lots with parcel-aware placement for homes, storefronts, apartments, schools, churches, parks, liquor stores, mini-malls and industrial yards.
- Deliver a streamed district assembly pass that reads like a real city fabric instead of a set of repeated sandbox blocks.

**Cycles 65-69: Environment art baseline and material realism**

- Move the world to a production PBR material pipeline for asphalt, concrete, painted surfaces, brick, stucco, glass, metal, chain-link, vegetation and weathering.
- Add decals, grime, patchwork asphalt, curb wear, graffiti, trash, poles, wires, hydrants, street furniture and utility clutter at a professional baseline.
- Replace placeholder lighting with calibrated sun, sky, exposure, shadowing, ambient occlusion and reflection behavior tuned for realistic outdoor readability.
- Build modular but visually rich architectural kits that avoid obvious repetition at street level and from driving speed.
- Milestone: end this phase with screenshot-quality streets and lots in at least one substantial Los Angeles district.

**Cycles 70-74: Large-world streaming and many-mile performance**

- Harden streaming for long uninterrupted driving across many miles with asynchronous loading, hierarchical LODs, impostors, memory budgets and hitch control.
- Add world partitioning for distant skyline layers, district silhouettes, freeway signage, tree lines, traffic density and pedestrian density at scale.
- Profile CPU, GPU, memory and bandwidth around realistic asset density rather than around graybox assumptions.
- Ensure that road continuity, district continuity and mission continuity survive long-distance traversal without visible seams or pop-in that would break immersion.
- Milestone: end this phase with a stable large-world traversal alpha that can cross a substantial portion of the linked Los Angeles gang-map footprint at professional visual quality.

**Cycles 75-79: Character, vehicle and traffic visual uplift**

- Replace placeholder pedestrians, gang members, police or opposition units and player character presentation with production-quality models, materials, wardrobe sets and animation blending.
- Rebuild vehicle visual quality for hero cars, bikes, motorcycles, traffic vehicles, damage presentation, headlights, interiors and reflections.
- Improve crowd composition, traffic composition and faction silhouette readability so the city looks authored rather than simulated with placeholders.
- Tune animation, camera behavior, suspension, tire placement, door interaction and street-level framing so traversal looks like a shipped game.
- Deliver a build where screenshots of traffic, pedestrians and roadside life no longer reveal prototype-quality characters or vehicles.

**Cycles 80-84: Atmosphere, time of day and cinematic presentation**

- Add professional atmosphere systems including improved sky, haze, fog layers, night lighting, emissive signage, interior window glow and heat or distance falloff.
- Build day, dusk and night presentation passes that maintain realism across roads, alleys, districts and skyline views.
- Upgrade city audio with district-aware ambience, traffic beds, sirens, far gunfire, pedestrian chatter, radios and environmental reverb tuned for immersion.
- Add cinematic camera framing, stronger post-processing discipline and visual storytelling rules for landmark corridors and mission-critical spaces.
- Milestone: end this phase with a visually convincing, atmospheric city slice that looks and sounds like a professional game rather than a technical demo.

**Cycles 85-89: Territory-authentic content expansion**

- Expand faction territories, ambient encounters and mission geography across the larger map using the Los Angeles gang-map reference as a layout guide for where pressure and identity should live.
- Re-author missions, spawns, patrol logic and mission routes around real road patterns, freeway access, alleys, dead ends, commercial corridors and neighborhood transitions.
- Add district-specific landmarking, street-name logic, minimap or world-map readability and navigation flows that support many-mile play.
- Remove any remaining graybox-only districts, placeholder signage or clearly artificial neighborhood transitions from the main playable area.
- Deliver a content-complete city pass where the larger map supports both free roam and mission structure without feeling procedurally empty.

**Cycles 90-94: Professional UX, polish and certification-level hardening**

- Rework HUD, pause, map, settings, onboarding and accessibility so the presentation quality matches the improved world.
- Add a polished world map that reflects the larger linked Los Angeles gang-map road coverage, district naming and mission routing.
- Run focused polish passes on collision, AI navigation, streaming edges, mission handoffs, vehicle routing, spawn masking and save stability in the realistic large world.
- Capture benchmark visual targets and close the gap between in-engine results and the intended professional-quality bar.
- Milestone: end this phase with a beta-quality build that looks, feels and navigates like a professional open-world game slice.

**Cycles 95-99: Regional beta foundation and full-map production prep**

- Lock the streaming, save, mission, routing, and profiling foundations needed to expand from a high-quality many-mile slice to the full linked Los Angeles gang-map footprint.
- Eliminate the remaining blockers that would make a whole-map build unstable, including hitching under longer routes, weak world-map readability, fragile encounter persistence, and district-to-district seam issues.
- Convert the current “beta-quality slice” goal into a “beta-quality regional foundation” goal so the team is not pretending the project is done while large portions of the required city footprint are still missing.
- End this phase with a stable regional build that proves the current toolchain can scale to full-map production instead of stopping at a limited-area ship candidate.

**Post-cycle-99 scope expansion**

- The previous 100-cycle plan assumed that a high-quality large-world slice might be enough to call the game complete.
- That is no longer the bar. The linked Los Angeles gang map is now the required playable footprint, not just a thematic guide.
- The next 25 cycles are reserved for full-map reconstruction, whole-city realism, and ship-quality finishing across the entire represented Los Angeles area.
- No district that still reads as blockout, no major corridor that is still schematic, and no obviously placeholder vehicle, character, or streetscape element counts as acceptable completion.

**Cycles 100-104: Full gang-map geo coverage and corridor completion**

- Ingest or reconstruct the entire linked Los Angeles gang-map footprint into production-ready world partitions, corridor classes, district definitions, and route metadata instead of stopping at a representative subset.
- Complete continuous major-road, arterial, freeway, alley, frontage-road, and neighborhood-connector coverage across the whole required map so long-distance travel reflects the real city-scale layout.
- Extend street naming, district naming, map overlays, encounter zoning, and mission-routing hooks across the full playable footprint so the app can reason about the whole map coherently.
- Deliver a build where the entire gang-map extent is navigable as a connected Los Angeles road skeleton rather than as a premium core surrounded by missing space.

**Cycles 105-109: Full-map parcel, block, and building reconstruction**

- Replace remaining large-form block placeholders across the full map with parcel-aware lot subdivision, realistic block depth, district-specific setbacks, parking logic, and alley or service conditions.
- Build production neighborhood kits for the major residential, mixed-use, commercial, civic, industrial, and freeway-edge patterns needed across the whole linked map footprint.
- Push skyline, block massing, curb furniture, fencing, retaining walls, transit edges, and streetscape density until no major district still reads as broad box composition from street level or from driving speed.
- Milestone: end this phase with the full Los Angeles gang-map footprint reading like a constructed city fabric instead of a partly dressed road skeleton.

**Cycles 110-114: Whole-city simulation, faction deployment, and mission geography**

- Scale traffic, pedestrian life, ambient behaviors, gang territory presence, mission anchors, and district-specific encounter logic across the full world so the complete map feels inhabited instead of only the original core.
- Re-author faction boundaries, social pressure, mission routes, and traversal beats to align with the entire linked gang map rather than with only the initial South Los Angeles slice.
- Harden AI navigation, pathfinding costs, spawn masking, and long-route continuity so systems remain believable across many-mile cross-city travel.
- Deliver a build where the entire required Los Angeles footprint supports free roam, systemic pressure, and mission geography without collapsing into sparse filler between hero zones.

**Cycles 115-119: Full-map art consistency and professional presentation**

- Unify lighting, materials, decals, vegetation, weathering, signage, skyline layering, atmosphere, and district visual identity across the entire map so quality does not drop outside a few polished corridors.
- Remove the remaining blocky or obviously placeholder forms in buildings, props, vehicles, characters, sidewalks, medians, and roadway edges until the whole city can hold up to close camera framing.
- Finish the professional world-map, navigation, HUD, and presentation passes so the user experience matches the scale and fidelity of the enlarged Los Angeles world.
- Milestone: end this phase with a content-complete, professional-quality full-map city pass that covers the whole linked gang-map footprint.

**Cycles 120-124: Final optimization, QA, and ship readiness for the full Los Angeles map**

- Optimize long-session play across the entire world for streaming stability, memory use, GPU cost, traffic load, pedestrian load, and systemic mission activity on target hardware.
- Run final bug burn-down for traversal, combat, AI, traffic, streaming, save or load, UI, territory logic, and mission continuity at full-map scale instead of only around a smaller representative slice.
- Capture and compare the game against professional visual and technical targets, and keep cutting weak spots until the whole city meets the bar rather than only the strongest districts.
- Prepare release-quality capture, documentation, credits, packaging, and final ship review only after the full linked Los Angeles world meets the realism and professionalism requirements.

**Milestone gates**

- End of Cycle 24: first playable vertical slice
- End of Cycle 49: internal alpha
- End of Cycle 59: many-mile Los Angeles road foundation in place
- End of Cycle 74: large-world traversal alpha at production visual density
- End of Cycle 89: content-complete realistic city pass
- End of Cycle 94: beta-quality professional regional world slice
- End of Cycle 99: stable regional beta foundation for full-map production
- End of Cycle 104: full linked gang-map road and district coverage in engine
- End of Cycle 109: full-map parcel and building reconstruction baseline
- End of Cycle 119: content-complete professional full-map city pass
- End of Cycle 124: ship target or disciplined continuation based on full-map quality bar

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
- After breaking the line, stay inside for the new follow-through beat and confirm it now resolves into a held-pocket outcome instead of dropping straight back into generic claimed-turf pressure. The prompt, encounter card, and street card should all acknowledge that the break stuck.
- Once that held-pocket outcome lands, stay with the block for a moment and confirm the aftermath now reads as a deeper reclaim on the same remembered shoulder instead of generic lingering pressure. The runner/post/lookout mix should keep reclaiming inside the pocket through that side, and the lookout cue or street card should lean into that same shoulder instead of snapping back to a neutral answer.
- Break the line again, then back out through the lane mouth before the pocket settles and confirm the new pullout path becomes readable as its own beat instead of sharing the same messaging as a held pocket.
- Finish that pullout cleanly and confirm the lookout now reads your exit while the street card acknowledges the clean pullout and short settling window rather than treating it like a failed commit.
- After that clean pullout lands, watch the lane mouth and confirm the aftermath now reads as a short edge retake or reseal on the remembered shoulder instead of the same reclaim behavior as the held pocket. The runner and post should answer at the mouth through that side while the lookout search and street settling stay attached to the same shoulder-led edge response.
- Let the held-pocket reclaim settle, then back out just long enough for the resolve beat to clear and re-approach quickly. Confirm the next pass now reads as a reclaim return: the lane mouth should feel a little looser, but the inner post and lookout should wake deeper and sooner than on a cold pass.
- On that reclaim-return pass, hover at the lane mouth first from one side, back off, and test it again in a shallow loop from that same side. Confirm the block now remembers that shoulder: the runner should counter-step off the same left or right shoulder while the inner post turns the deeper angle there, and the prompt or card should call that shoulder out instead of reading as a generic reclaim feint.
- Then take that reclaim-return pass deeper after one of those shallow loops and confirm the softer edge now flips into a faster deeper reclaim commit on that same shoulder instead of replaying the same mouth beat. The encounter or street readout should acknowledge that the deeper handoff kept carrying the remembered shoulder on the second beat.
- Hold that same-side reclaim long enough to settle and confirm the lookout anchor and the street card now bias toward the same remembered shoulder instead of leaving the broader response centered or generic while the runner/post handoff is shoulder-specific.
- Let the clean-pullout edge retake settle, then turn back in quickly and confirm the next pass now reads as a retake return: the runner and post should answer earlier at the lane mouth, and the prompt or street card should acknowledge that earlier edge read instead of replaying the same neutral approach.
- On that retake-return pass, test the lane mouth without fully driving through it, back off, and press it again from the same side. Confirm the block now remembers that left or right shoulder and throws the harder earlier edge challenge there before full recommit. The runner and post should feel like they are checking the return from complementary angles on that remembered shoulder instead of instantly skipping to the old deeper pressure.
- Push through that repeated retake challenge and confirm it now flips into a clearer recommit through the mouth on that same shoulder rather than staying in one static brace. The prompt or card should acknowledge that the edge challenge turned into a retake commit and reseal on the remembered side.
- Finish a same-side clean pullout and confirm the lookout search and street retake/reopening read now stay attached to that same remembered shoulder instead of reporting only a generic exit search or neutral reseal.
- Stay with either same-side held-pocket reclaim or same-side clean-pullout reseal until the live watch falls off and confirm the street card plus territory summary keep the same shoulder through normalization instead of dropping it as soon as the direct pressure clears.
- Let that normalization pulse roll into reopening and confirm the street card, lookout summary, and territory summary still tag the same left or right shoulder while traffic and civilians reopen the block instead of flattening back into a neutral cooldown.
- As that reopening starts, hover back near the lane mouth from the same side and confirm the runner/post reform now gathers around that same remembered shoulder instead of reforming from a neutral center. The prompt, encounter card, and territory summary should all keep calling out the same shoulder while the edge reforms.
- Stay in a staged vehicle for one of those reopen passes and confirm the same shoulder now carries the vehicle-side reform language too instead of only the on-foot reform phrasing.
- Watch the nearby road link and shoulder-side spillback during that reopen beat and confirm the street card now reads as the same shoulder reopening and reforming there instead of a generic whole-block reset.
- Let that explicit reopening timer finish, stay near the block, and confirm the runner/post pair now settle into a softer late fallback on that same shoulder instead of collapsing straight back to neutral center the moment recovery ends.
- Begin the next quiet boundary approach from that same side after reopening has already ended and confirm the first neutral watch still leans onto the remembered shoulder instead of reintroducing the line from a centered default.
- Watch the prompt, street card, lookout summary, and territory summary during that late fallback beat and confirm they keep calling out the same shoulder as a fallback or settle instead of going silent until the next hot re-entry.
- Let that late fallback almost drain, then ease back into the lane mouth from the same side and confirm the first truly cold screen or clamp still forms on that remembered shoulder instead of restarting from a neutral center.
- Watch the prompt, street card, lookout summary, and territory summary on that first cold return and confirm they now call out the same shoulder as a cold carry rather than dropping back to generic first-contact wording.
- Let that same cold return begin one step farther out and confirm the first fully neutral outside watch now also leans onto the remembered shoulder before the screen or clamp commits instead of beginning from a centered default.
- Watch the prompt, street card, lookout summary, and territory summary during that softer outside-watch beat and confirm they now call it out as a same-shoulder watch carry rather than skipping straight from fallback to cold screen/clamp language.
- Start that same return even farther out and confirm the runner now drifts off idle into the remembered shoulder before the outside watch fully forms instead of waiting to wake until the later watch beat.
- Watch the prompt, street card, lookout summary, and territory summary during that earliest outer-post drift and confirm they now call out the same shoulder as an approach carry rather than skipping straight to outside-watch wording.
- Re-enter during that reopening window from the same side and confirm the hot-territory prompt now calls out that same shoulder as the fast read instead of only generic burned-block language.
- Swap sides on a later reclaim or retake return and confirm the preferred shoulder can migrate rather than staying permanently glued to the first remembered side. The prompt, street card, and territory summary should all update with the new left or right shoulder once the loop has clearly shifted.
- Back out too early during that beat and confirm the objective collapses cleanly back into the existing runner/post territory behavior instead of hanging around after the line has settled.
- Push farther into claimed turf and confirm the sidewalk runner now clears the line while the inner post owns the pocket, with the encounter readout shifting from clamp or screen language into clear/post pressure once the handoff is live.
- Watch the inner post during that retake and confirm it now has a clearer completion rhythm: farther back it should relax more, while a closer recommit should make it step up harder instead of feeling equally strong at every distance.
- Back off once the lookout search and civilian panic have ended, and confirm the sidewalk runner settles first while the inner post lingers deeper for a beat before the pair fully cools back toward idle.
- Repeat the approach from a slightly different lateral angle or after a hot return and confirm the runner and inner post do not frame the same line the same way every time. The edge screen, clamp, clear-line beat, reform, harden, and deeper support pocket should feel like a small staged setup instead of one rigid placement.
- Stay with the block after the normalization pulse fades and confirm the street now passes through a short reopening phase instead of dropping straight to idle. The street card should call out reopening while nearby traffic retakes the lane and adjacent link in a softer, cooler handoff.
- Provoke the lane, pull out of the territory, then re-enter before it cools and confirm the block is now received as hot territory in a readable paired sequence: the runner should recover first, the inner post should take the handoff quickly, the street card should call out a burned block, and the hostile should pick you up faster instead of replaying the same cold start.
- Treat the whole pass as the first paired-fallback check: sidewalk runner, inner post, on-foot line skim, vehicle line skim, deeper push, runner-to-post handoff, staggered fallback, hot re-entry, and the existing witness/traffic spillover should now read as one socially occupied slice instead of one lookout plus labels and markers.
- Fire until the clip runs low, press `Y` to reload, and confirm the prompt pill, health card, weapon card, encounter card, and street card stay readable while hostile movement, multi-civilian reaction, traffic reaction, and staged vehicle handoff prompts overlap.
- Walk or drive north and east out of the original sandbox and confirm the wider graybox now holds a named sixteen-block Los Angeles reference grid instead of stopping after the original four-block slice.
- Watch the HUD map card and floating debug panel while moving across the grid and confirm they now call out named districts, streaming tiles, and corridors such as Crenshaw Blvd, Jefferson Blvd, Exposition Blvd, Western Ave, or Vermont Ave instead of only generic west-grid or east-grid labels.
- Walk the north-south corridors and confirm the avenues now read differently from the east-west boulevards: avenues should show curb-lane bands and boundary striping, while boulevards should carry a stronger center median and wider intersection read.
- Move through one of the service-spur mixed-use blocks and confirm the frontage now shows clearer loading-zone paint and service-lane staging instead of reading like the same curb treatment as the civic or residential edges.
- Pull farther back in third person and confirm the static world keeps the wider corridor skeleton visible at once, with the road hierarchy now reading as boulevard-versus-avenue structure instead of one uniform street kit stretched across the whole map.
- Walk from one east-west boulevard onto a north-south avenue and confirm the street now changes cross-section before the markings even matter: boulevards should feel broader with deeper sidewalks, while avenues should tighten up sooner at the curb.
- Cut diagonally across road, curb, and sidewalk on foot or on a two-wheel vehicle and confirm the ground height and grip transition now follow the road class instead of feeling like one fixed curb ramp reused everywhere.
- Drive a sedan or coupe down both corridor types and confirm lane pull now settles a little wider on boulevards and a little tighter on avenues instead of centering every road with the same travel offset.
- Pause at several intersections across the map and confirm the node kit no longer repeats one square layout: crosswalk spacing, stop bars, lane-arrow stand-off, signal-pole setback, and planter spacing should now shift with the named corridor pair.
- Compare a Mid City West or Expo Crenshaw node against a Central South or Florence Vermont node and confirm the corner pads feel slightly broader and greener in the western tiles while the southern tiles stay a little tighter and harder in their block-edge read.
- Drive up a boulevard approach and confirm the new refuge islands now shape the run-in before the node instead of leaving every median to disappear into the same flat intersection mouth.
- Compare a civic retail block, a transit-market block, a residential court block, and a service-spur block and confirm the shopfront band, furniture lane, shelter setback, and rear-edge spacing no longer sit on one universal frontage depth.
- Move between western tiles and southern tiles while watching those same frontage types and confirm the chunk or corridor bias now nudges awning span, planter size, rear fence depth, and loading-zone offset instead of every block face reusing the same storefront kit.
- Check the service-spur mixed-use blocks specifically and confirm the painted loading zones, curb furniture, and rear fence line now stay coordinated with the service frontage profile instead of floating on the old fixed offsets.
- Walk past ambient pedestrians, the witness, the bystander, and the territory edge agents in third person and confirm they now read with shoulders, separate legs, arms, necks, and heads instead of the older stacked-box mannequin silhouette.
- Compare staged traffic, ambient traffic, and parked street cars and confirm the bodies now taper into hood, cabin, and roof layers instead of reading as two rectangular slabs.
- Stop beside a coupe, sedan, bike, or motorcycle and confirm the wheels now read as layered rounded tires with visible hubs rather than as single cube blocks.
- Treat this whole pass as the first post-milestone faction check: traversal, staged vehicle escape, lane combat, civilian spillover, traffic easing, normalization, reopening, territory crossing, and hot re-entry should now read as one socially structured graybox slice instead of a neutral sandbox.

The current prototype is a macOS graybox sixteen-block Los Angeles reference grid with four authored streaming tiles, named district metadata, named arterial and avenue corridor metadata, shared road-spine descriptors plus road-class cross-section profiles that now drive road surfaces, curb width, sidewalk depth, lane markings, lane pull, and ground queries from the same metadata, explicit corridor and world-chunk descriptor tables that now steer intersection spacing and block-edge treatment, a frontage descriptor layer that now blends frontage template with corridor and chunk bias for shopfront setback, furniture lane spacing, transit-stop stand-off, rear fence depth, awning span, and service loading-zone placement, boulevard-versus-avenue street treatment, intersection-aware crosswalk or stop-bar spacing, corner plaza pads, signal setbacks, and boulevard refuge islands, clearer service-lane loading-zone readouts in the spur blocks, frontage-template-driven block variants, hotspot hooks, engine-exported traffic occupancy hooks, route-aware placeholder traffic with stop-zone holds and early near-miss hazard pulses, staged sedan, coupe, moped, bicycle, and motorcycle handoffs with ranked selection cues and lock controls, richer non-player civilian and patrol silhouettes with layered shoulders, torsos, arms, legs, necks, and heads instead of simple stacked-box mannequins, and a broader vehicle form pass that now gives staged, ambient, and parked traffic more believable hood, cabin, roof, and rounded-wheel layering rather than two-box car bodies. The combat scaffold still includes a lead-pipe pickup, a pistol pickup, weapon-slot switching, ammo and reload state, muzzle and impact feedback, cover-aware firearm blocking, a lookout hostile that can return fire, pressure-anchor-driven hostile repositioning, two authored civilian reaction sources around the lane, ambient pedestrians that can peel away from live incidents, a lightweight hostile last-seen search state, a persistent street-incident layer that spills into the nearest adjacent road links, a distinct street-reopening handoff after normalization, warmer spillover and cooler normalization/reopening ambient-link feedback, and the first faction-territory scaffolding with a readable court-set boundary at the lane mouth, claimed-turf readouts inside the block, a visible court-set sidewalk runner, a second ambient inner post replacing the old deep marker, simple runner-to-post handoff rules, a clearer sidewalk screening beat before full territory commitment, a brief complementary clamp at the boundary when the player hesitates, a lightweight clear-line beat once the inner post or lookout fully owns pressure, a lightweight reform/re-approach beat when the player backs out, a lightweight harden/brace beat when the player recommits during reform, a lightweight lane-mouth commit window once that hardened line is live, a short push-through objective progress and completion beat without a separate mission panel, a tiny follow-through resolve after the line break where the player can either briefly hold the pocket or pull out cleanly, distinct short aftermaths for those two outcomes, a short next-pass memory after each one, a slightly richer edge texture on those returns where held pockets can show a softer reclaim feint before the deeper pocket commits and clean pullouts can throw an earlier edge challenge before recommitting through the mouth, a more active repeated-half-commit answer on top of that texture so shallow recommits and backing-off loops can keep the reclaim side baiting deeper pressure while the retake side hardens the mouth check sooner, a small positional answer on top of those edge beats so the runner can counter-step off the softer mouth while the inner post turns the deeper angle and the retake side can cross-angle the lane mouth before the heavier recommit lands, and now a lightweight remembered shoulder on top of that so repeated left/right approaches can bias which side of the mouth the runner and post answer through on reclaim and retake returns, with that same shoulder now carrying farther through deeper reclaim handoffs and edge reseals instead of flattening back into a neutral follow-through, leaning the lookout anchor plus the broader street/search response toward that same shoulder during held-pocket reclaim and clean-pullout reseal, staying readable through normalization, reopening, and hot re-entry instead of dropping that shoulder memory as soon as the lane cools, pulling reopening spillback plus runner/post boundary reform toward that same shoulder so the block does not reopen from a generic center, leaving a softer same-shoulder late fallback behind after reopening fades so the next quiet line watch does not reset from a neutral default, carrying that remembered shoulder into the first fully cold screen or clamp so the next fresh boundary catch does not snap back to center either, letting that same carry survive one beat earlier into the first neutral outside watch before the colder screen or clamp forms, and now letting it survive one beat earlier again into the runner’s first far-out drift off idle before that outside watch fully forms. The in-scene combat HUD now carries both that territory state and a dedicated map card with district, tile, and corridor readouts alongside the street/system card, so player health, recovery, systemic pressure, faction presence, sidewalk watch, entry screen, entry clamp, clear-line handoff, reform, hardening, commit window, line break, hold-versus-pullout resolve, pocket reclaim, edge retake, reclaim return, reclaim feint, reclaim commit, retake return, edge challenge, retake commit, territory crossing, hostile escalation, and named world context can be read together instead of as unrelated debug fragments. Use the on-screen overlay and the floating controls window together for controls, chunk visibility, staged vehicle prompts, combat state, street pressure, traffic reaction, civilian reaction, territory heat, watch, patrol alert, inner-post alert, handoff ranking and lock state, grip, lane, bump, recovery, hazard feedback, and active block metadata.

### Any questions ###

Contact barbalet at gmail dot com
