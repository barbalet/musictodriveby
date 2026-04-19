# musictodriveby

## Description

MusicToDriveBy is a first and third person perspective open-world Los Angeles environment generator allowing player to move from first person perspective to third person perspective while being on foot, on bicycle, on motorcycle, moped or car. The game also allows for melee weapon use including rocks, knives, machetes, pistols, revolvers, rifles, sawn off shot guns, shot guns, AK47s and M16s with the related caliber bullets, shotgun shells and rifle cartridges and clips.

The environment is the full Los Angeles gang-map footprint identified here:

https://www.google.com/maps/d/u/0/viewer?mid=1ul5yqMj7_JgM5xpfOn5gtlO-bTk&ll=33.98203848816218%2C-118.45161449214874&z=9

The game is aesthetically similar to the Grand Theft Auto series of first and third person perspective games.

The game is written in Metal with SwiftUI and a C engine core.

Please plan for the cycles needed to create this game allowing for 125 cycles or so until completion.

### Current Cycle 

116

Cycle 115 was completed on April 19, 2026.
See `docs/cycles/cycle-115.md` for the cycle report.

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
- Compare side-building masses across hub, mixed-use, and residential blocks and confirm they now read with parapets, plinths, glazing bands, corner trim, and small rooftop kit instead of as bare rectangular masses.
- Inspect the transit shelters and confirm they now read with a four-post frame, roof stack, partial glass enclosure, ad panel, and curb rail instead of as a single roof slab with one wall.
- Check benches, planters, bollards, and the corner-store landmark and confirm the curb furniture now reads as a layered street kit rather than as isolated one-box props.
- Walk a retail or mixed-use frontage and confirm the shared awnings now read with deeper valances, end caps, glazing bands, and support rods instead of as one thin slab with two posts.
- Stop at several intersections and confirm the signal poles now carry concrete bases, mast-arm hardware, and visible signal heads rather than a bare post with one floating crossbar.
- Compare trash bins, newsstands, and the apartment entry against nearby shelters and storefronts and confirm those roadside props now read with lids, wheel housings, framed display faces, stoops, and door or canopy detail instead of reverting to simple filler boxes.
- Walk the outer side lots on retail, service-spur, and residential blocks and confirm they now read with asphalt pads, curb edges, stall striping, wheel stops, and planter islands instead of as broad flat dirt wings.
- Look along the back-lot edges and confirm the new utility-pole runs and overhead wire spans now give the block perimeter a taller Los Angeles service silhouette instead of leaving those edges visually blank.
- Compare a residential court block against a service-spur block and confirm the shared lot kit now shifts in pad size and surface weight rather than applying one identical parking read to every side parcel.
- Walk a retail or mixed-use frontage and confirm the storefronts now carry tile or kickplate bands, recessed entries, transom bands, and clearer mullions instead of reading as one glass strip under the awning.
- Stand inside the new side-lot pads and confirm the asphalt now carries tire-darkening, repair patches, faded stall paint, and oil marks instead of looking uniformly fresh across every parcel.
- Compare storefront and parking treatment across western and southern blocks and confirm the city now reads more used-in and materially varied rather than like one clean procedural kit repeated at every edge.
- Stop beside several shelters and confirm each stop now reads with a separate route sign and nearby utility cabinet rather than ending at the glass box alone.
- Walk hub, mixed-use, residential, and service-spur curbs and confirm the frontage edges now carry parking meters, hydrants, and cabinet clutter in plausible positions instead of reverting to long clean sidewalks.
- Pause at multiple intersections and confirm the corners now pick up signal-control boxes and hydrants alongside the richer signal poles so the node reads like a serviced city corner instead of only a landscaped one.
- Walk along several boulevard and avenue curb runs and confirm the roadway now carries darker gutter bands at the road edge instead of one flat asphalt slab meeting the curb.
- Stop at non-intersection curb stretches and confirm storm drains now repeat along the shared corridors rather than service detail appearing only at shelters or corners.
- Compare boulevard and avenue corridors and confirm the new service covers sit in different lane positions instead of every road reusing one generic center patch.
- Pause at several intersections and confirm the corner edge now rounds outward through stepped curb-return geometry instead of holding a hard square sidewalk cut at every mouth.
- Approach a node from multiple directions and confirm the new lane-mouth patches now step the edge lanes into the intersection instead of snapping from straight corridor to perfect square opening.
- Compare boulevard and avenue entries and confirm the new transition patches inherit each road class’s width and stop-bar geometry rather than reading like one shared pasted-on apron.
- Stop in several intersection centers and confirm the node now carries a darker center patch and seam breakup instead of reading as one flat repeated asphalt box.
- Walk onto multiple crosswalk landings and confirm the sidewalk now steps into the node through brighter landing pads rather than dropping straight from curb return to generic sidewalk fill.
- Compare boulevard and avenue nodes and confirm the new center-wear pads still follow each corridor’s crosswalk and stop-bar proportions instead of reading like one fixed intersection stencil.
- Stop beside boulevard refuge islands and confirm they now carry capped ends and a central seam instead of reading as plain raised boxes with one planter.
- Approach those refuges through the crosswalk zones and confirm the surrounding road now picks up short stripe clusters that make the island approaches feel intentionally marked instead of abruptly cut out of the asphalt.
- Compare north-south and east-west boulevard refuges and confirm the new cap and stripe treatment rotates with the island orientation rather than reading like one pasted-on decal layout.
- Stop at several signalized crosswalks and confirm the white bars now sit inside darker edge skirts with short outer caps instead of floating directly on flat asphalt.
- Stand beside the signal poles at multiple corners and confirm the sidewalk now picks up small wait-pad surfaces with seams rather than leaving the signal corners as one uninterrupted plaza slab.
- Drive through boulevard refuge approaches and confirm the asphalt between each crosswalk and refuge now carries a darker throat patch with guide marks instead of breaking from stripe field straight into the raised island.
- Pause at multiple stop bars and confirm each one now sits on a darker shoulder patch with end-shoulder structure instead of reading as a single thin white line floating over plain asphalt.
- Compare boulevard and avenue approaches and confirm the new stop-bar shoulders scale with the lane width instead of pasting one identical shoulder stencil onto every node mouth.
- Pull up to the lane arrows on all four approaches and confirm each arrow now sits on a darker stand-off pad with a slim seam and side guides rather than appearing as an isolated painted symbol.
- Drive a boulevard approach from the arrow zone toward the stop bar and confirm the center divider lines now carry a darker throat strip with painted caps instead of dropping out between the arrow pad and the stop-bar shoulder.
- Compare north-south and east-west boulevard entries and confirm the new divider throat structure rotates with the corridor orientation instead of reading like one pasted-on marking stencil.
- Compare a boulevard node against an avenue node and confirm only the boulevard approaches pick up this paired divider-throat language, while avenues keep the simpler control layout.
- Stop beside several signal control boxes and confirm each one now sits on a darker service pad with a seam rather than landing directly on one uninterrupted plaza slab.
- Look from each signal pole back toward its nearby control box and confirm the sidewalk now carries conduit-style service runs with a small pull-box cover instead of leaving the corner hardware visually disconnected.
- Check the hydrants at those same signalized corners and confirm they now sit on small utility footings instead of dropping straight onto plain sidewalk.
- Drive toward the center arrows on several approaches and confirm the asphalt between each stop bar and arrow now reads as one darker turn-pocket surface instead of a clean gap between separate control marks.
- Compare boulevard and avenue approaches and confirm the new turn-pocket surface widens or narrows with the lane offset rather than stamping one identical pocket shape onto every node.
- Look down north-south and east-west approaches and confirm the new pocket seams and shoulders rotate with the corridor axis instead of reading like one pasted-on stencil.
- Stop beside several signal control boxes and confirm the service pads now carry darker edge-wear borders instead of reading as perfectly clean inset rectangles.
- Follow the conduit runs from pole to cabinet and confirm the surrounding sidewalk now shows a broader scuffed service strip rather than only a single thin line.
- Check the hydrant footings at those same corners and confirm each one now carries a darker worn surround instead of sitting on a single small bright pad.
- Walk onto several signalized corners and confirm the wait zones now carry tactile warning pads and short approach bands instead of ending as plain bright sidewalk rectangles at the curb.
- Compare both crossing directions at multiple corners and confirm the tactile warning pads rotate with the crosswalk axis rather than reading like one pasted strip layout.
- Look down at the curb returns beside those same corners and confirm the ramp landings now carry seam scoring and a small center break instead of one smooth uninterrupted sidewalk patch.
- Stop beside several signal poles and confirm each one now sits on a darker scored footing apron instead of planting straight into plain sidewalk.
- Look around the inside faces of those same poles and confirm each corner now carries two small pedestrian call boxes aligned to the two crossing directions rather than one generic pole with no control hardware.
- Compare opposite corners and confirm the new call-box layout flips with the corner orientation instead of reading like one pasted hardware arrangement.
- Look up at several mast heads and confirm each signal now carries a visible hanger and top clamp instead of reading like a clean box simply floating under the arm.
- Compare the near head and the outer heads on the same pole and confirm the new mount hardware stays consistent across all three placements rather than only dressing one hero head.
- Compare north-facing and south-facing poles and confirm the new support hardware flips with the mast orientation instead of reading like one pasted bracket layout.
- Look where the mast arm leaves the pole and confirm each signal now carries a visible collar and short reinforcement brace instead of reading like one clean beam directly plugged into the post.
- Compare opposite mast directions and confirm the new reinforcement flips with the arm orientation instead of reading like one fixed gusset pasted onto every pole.
- Compare several districts and confirm the reinforcement layer stays subtle but readable rather than turning into a one-off hero mast treatment.
- Stand beside the inward-facing sides of several signal poles and confirm each corner now carries small paired pedestrian-signal readouts above the call boxes instead of stopping at button hardware alone.
- Compare the x-facing and z-facing readouts on the same corner and confirm the new housings rotate with the crossing direction instead of reading like one pasted display slab.
- Look for the amber upper readout and lighter lower readout on those same housings and confirm the display break-up stays visible without overpowering the existing pole hardware.
- Look along several mast arms and confirm each one now carries a darker secondary cable trunk with small clamps instead of reading like a clean uninterrupted beam between the pole and heads.
- Compare the center head and both outer heads on the same mast and confirm each one now picks up a short cable drop instead of only the arm surface carrying detail.
- Compare opposite mast directions and confirm the new wiring run flips with the arm orientation instead of reading like one pasted cable layout.
- Look up at several signal heads and confirm each lens now carries a short visor instead of the face reading like three bare light slabs set into a flat box.
- Compare the upper and lower portions of those same heads and confirm the lighter sun-fade band and darker lower grime band stay visible without overpowering the lens colors.
- Compare opposite-facing mast heads and confirm the new visor and face-weathering detail follows the head orientation instead of reading like one pasted front decal.
- Circle behind several signal heads and confirm each housing now carries a subtle rear access panel instead of reading like one clean back slab.
- Compare the upper and lower rear portions of those same heads and confirm the new horizontal seam bands stay visible without overpowering the broader housing silhouette.
- Compare opposite-facing mast heads and confirm the rear panel and latch detail follows the head orientation instead of reading like one pasted back decal.
- Walk past several signal heads at an oblique angle and confirm each housing now carries a subtle side hinge spine instead of dropping straight back to a clean box edge.
- Compare the upper and lower portions of that same side detail and confirm the new hinge caps stay readable without overpowering the overall head silhouette.
- Compare opposite-facing mast heads and confirm the side hinge detail flips with the head orientation instead of reading like one fixed world-space strip.
- Circle behind several signal heads and confirm each housing now carries a small rear conduit coupling near the upper back instead of leaving the service entry implicit.
- Compare that same coupling against the rear access panel and confirm the new collar reads as a separate connection point rather than disappearing into the backplate.
- Compare opposite-facing mast heads and confirm the rear conduit coupling follows the head orientation instead of reading like one pasted rear decal.
- Look up at the underside of several signal heads and confirm each housing now carries a small lower drain lip instead of ending in one clean flat bottom edge.
- Compare the forward and rear portions of that lower edge and confirm the brighter front lip and smaller rear outlet read as one shared drainage detail rather than random underside noise.
- Compare opposite-facing mast heads and confirm the lower drain detail follows the head orientation instead of reading like one fixed world-space underside strip.
- Look where the mount meets several signal heads and confirm each one now carries a small compressed gasket pad instead of the hardware reading like a clean direct metal-to-metal join.
- Compare the center and outer heads on the same mast and confirm the new top gasket and small side tabs stay consistent across all three placements rather than only dressing one hero head.
- Compare opposite-facing mast heads and confirm the mount gasket detail follows the head orientation instead of reading like one fixed world-space strip above every signal.
- Stand beside several signal heads and confirm the backplate now reads with a small offset shim layer instead of sitting as one clean slab directly behind the housing.
- Compare the side edges and top edge of that same backplate and confirm the new shim pieces stay subtle but readable instead of turning into a second oversized frame.
- Compare opposite-facing mast heads and confirm the backplate shim detail follows the head orientation instead of reading like one pasted spacer layout.
- Look closely at the mount clamp on several signal heads and confirm it now carries a small center seam and end-cap breakup instead of reading like one untouched clamp block.
- Compare the upper and lower clamp blocks on the same head and confirm both now pick up the shared seam language rather than only the top clamp reading finished.
- Compare opposite-facing mast heads and confirm the clamp seam detail follows the head orientation instead of reading like one fixed world-space mark on every mount.
- Look closely at several signal-head corners and confirm each housing now carries small corner fastener hints instead of reading like a perfectly clean cast box.
- Compare the upper and lower corners on the same head and confirm the new fastener hints stay balanced and subtle rather than overpowering the existing visor and lens breakup.
- Compare opposite-facing mast heads and confirm the corner-fastener detail follows the head orientation instead of reading like one pasted front decal.
- Look closely at the visor sides on several signal heads and confirm each visor now carries a small side-attachment breakup instead of reading like one uninterrupted folded shade.
- Compare the side attachments against the visor cap and side returns and confirm the new straps and caps stay subtle rather than overpowering the visor silhouette.
- Compare opposite-facing mast heads and confirm the visor-side attachment detail follows the head orientation instead of reading like one pasted side decal.
- Look along the body side of several signal heads and confirm each housing now carries a small service-tab hint instead of reading like a perfectly smooth side panel.
- Compare the upper and lower parts of that side tab on the same head and confirm the tab body and tiny latch nubs stay subtle rather than turning into a second hinge treatment.
- Compare opposite-facing mast heads and confirm the side service-tab detail follows the head orientation instead of reading like one pasted side decal.
- Look at the face of several signal heads and confirm the new lens-baffle spacer breakup reads between the red, amber, and green lenses instead of the head face collapsing back into one open panel.
- Compare the horizontal spacer bars against the side rails on the same head and confirm the new face breakup stays subtle and structural rather than turning into a heavy front grille.
- Compare opposite-facing mast heads and confirm the lens-baffle spacer detail follows the head orientation instead of reading like one pasted front decal.
- Look at the upper rear of several signal heads and confirm the new vent hint reads as a small serviced cooling detail instead of the back panel collapsing into one uninterrupted slab.
- Compare the vent frame, dark slots, and tiny rain lip on the same head and confirm the new rear breakup stays subtle rather than turning into a heavy rear grille.
- Compare opposite-facing mast heads and confirm the rear-vent detail follows the head orientation instead of reading like one pasted rear decal.
- Look along the top shade of several signal heads and confirm the new visor-cap seams break the visor into a more assembled cap instead of one uninterrupted folded slab.
- Compare the small center cap plate against the left and right seam strips on the same visor and confirm the new breakup stays subtle rather than turning into a heavy three-piece badge.
- Compare opposite-facing mast heads and confirm the visor-cap seam detail follows the head orientation instead of reading like one pasted front-top decal.
- Look at the rear service panel of several signal heads and confirm the new fastener cluster reads as small maintained hardware instead of the back panel collapsing into one clean stamped piece.
- Compare the rear fastener washers against the tiny center fasteners on the same head and confirm the new cluster stays subtle rather than turning into a bold rear badge.
- Compare opposite-facing mast heads and confirm the rear-panel fastener detail follows the head orientation instead of reading like one pasted rear decal.
- Look up at the underside of several signal-head visors and confirm the new seam hint breaks the shade into a more believable folded assembly instead of one clean slab from below.
- Compare the dark underside join strip against the tiny side tie-ins on the same visor and confirm the new breakup stays subtle rather than turning into a heavy under-brace.
- Compare opposite-facing mast heads and confirm the visor-underside seam detail follows the head orientation instead of reading like one pasted underside decal.
- Look at the rear service entry on several signal heads and confirm the new coupling clamp breakup reads as a small assembled hold-down instead of one smooth conduit collar.
- Compare the darker clamp band against the tiny side tabs on the same coupling and confirm the new breakup stays subtle rather than turning into a heavy rear bracket.
- Compare opposite-facing mast heads and confirm the rear-coupling clamp detail follows the head orientation instead of reading like one pasted rear decal.
- Look along the lower drain lip on several signal heads and confirm the new seam hint breaks the edge into a more believable folded strip instead of one clean underside bar.
- Compare the dark lower seam against the tiny end caps on the same lip and confirm the new breakup stays subtle rather than turning into a heavy bumper strip.
- Compare opposite-facing mast heads and confirm the lower-lip seam detail follows the head orientation instead of reading like one pasted underside decal.
- Look at the head mount on several signal poles and confirm the new brace-edge breakup reads as fabricated support detail instead of two smooth brace blocks under the clamp.
- Compare the darker brace edge against the tiny brace caps on the same mount and confirm the new breakup stays subtle rather than turning into a heavy bracket overlay.
- Compare opposite-facing mast heads and confirm the mount-brace edge detail follows the head orientation instead of reading like one pasted support decal.
- Look at the three mast-head hangers on several signal poles and confirm the new seam hint reads as a fabricated hanger detail instead of three smooth drop bars under the arm.
- Compare the darker hanger seam against the tiny upper collar on the same drop and confirm the new breakup stays subtle rather than turning into a bold decorative band.
- Compare opposite-facing mast heads and confirm the hanger seam detail follows the arm orientation instead of reading like one pasted world-space strip.
- Look at the signal-arm collar blocks on several poles and confirm the new edge breakup reads as fabricated collar detail instead of smooth support blocks around the arm reinforcement.
- Compare the darker collar-edge bands against the tiny top cap on the same collar cluster and confirm the new breakup stays subtle rather than turning into a bold trim stripe.
- Compare opposite-facing mast heads and confirm the signal-arm collar detail follows the arm orientation instead of reading like one pasted world-space strip.
- Look at the wiring clamps along the signal arm and confirm the new seam hints read as strapped clamp hardware instead of smooth clamp blocks around the cable runs.
- Compare the darker clamp seams against the tiny top caps on the same clamp set and confirm the new breakup stays subtle rather than turning into a bold trim stripe.
- Compare opposite-facing mast heads and confirm the mast-wiring clamp detail follows the arm orientation instead of reading like one pasted world-space strip.
- Look at the pedestrian-signal mounts on several corners and confirm the new seam hint reads as fabricated mount hardware instead of one smooth mount block behind the readout.
- Compare the darker mount seam against the tiny center cap on the same mount and confirm the new breakup stays subtle rather than turning into a bold trim stripe.
- Compare opposite-facing pedestrian signals and confirm the mount seam detail follows the signal orientation instead of reading like one pasted world-space strip.
- Open the map card or debug overlay and confirm the engine now reports a much broader world skeleton with sixty-four blocks, nine tiles, and sixteen named roads instead of the older sixteen-block slice.
- Travel west-to-east across the widened grid and confirm corridor readouts now hand off through Fairfax, La Brea, Crenshaw, Arlington, Western, Vermont, Figueroa, and Central instead of stopping at the old four-column footprint.
- Travel south across the widened grid and confirm corridor readouts now hand off through Pico, Washington, Adams, Jefferson, Exposition, Martin Luther King Jr, Slauson, and Florence instead of ending near the previous central core.
- Push out toward the far west, far east, and deep south edges and confirm blocks, staging, road markings, and district or tile labels still render coherently instead of dropping into empty out-of-bounds space.
- Walk or drive through the widened outer tiles and confirm the new frontage or connector lanes appear as darker side-road bands with mouth connections instead of every block reading as one clean arterial rectangle between the main corridors.
- Step onto one of those connector lanes and confirm vehicle grip or road response now treats it like road space rather than generic lot fill when you cross onto the new side-lane surface.
- Visit several outer tiles and confirm the activity summary now picks up more hotspot and staged-vehicle coverage at the new edges instead of leaving those districts with only the core block hooks.
- Visit several outer districts and confirm the new branch or cut-through connectors now break off the shared outer side lanes at different offsets and on different sides instead of every edge block reusing the same straight frontage strip.
- Compare residential, transit-market, civic, and service-spur outer tiles and confirm the new branch-end landmark and hotspot hooks move to different route depths and side offsets instead of every edge district advertising activity at the same connector position.
- Move through a few widened outer tiles and confirm the street or system card now keeps showing low-intensity stop-zone pressure around those branch lanes and connector nodes so the enlarged routes feel passively occupied instead of visually added but systemically quiet.
- Visit multiple widened outer branches and confirm each one now ends in a visible route marker plus a small frontage support kit instead of fading from darker branch asphalt straight back into generic lot fill.
- Compare residential, transit-market, civic, and service-spur branch ends and confirm the new support props differ by district feel, with calmer residential street furniture, transit-oriented kiosks, heavier service hardware, and civic-retail route furniture instead of one repeated branch-ending prop cluster.
- Approach branch routes on foot and by vehicle and confirm the deeper branch-side vehicle and pedestrian handoff anchors now appear along those outer branches instead of leaving traversal choices centered only on the primary connector lane.
- Visit several widened outer branch ends and confirm the new branch-node hotspots and civilian pull points now cluster around the marked support nodes instead of treating every branch as only one deeper landmark and one branch-center hotspot.
- Compare residential, transit-market, civic-retail, and service-spur branch nodes and confirm the new dynamic route-signaling layer changes by district feel, with softer residential glows, transit-heavy marker lighting, warmer service cues, and civic-retail sign energy instead of one repeated branch-end accent.
- Move through the widened outer tiles and confirm the street or system summary now keeps a second low-intensity stop-zone beat around the marked branch support nodes, so those branch ends feel like lightly active route destinations instead of only a mid-branch pressure pulse.
- Visit several widened outer branch ends and confirm the visible support props, route accents, hotspot clustering, and sidewalk pull points now sit on the same support-node geometry instead of feeling like separate disconnected systems around the branch.
- Compare residential, transit-market, civic-retail, and service-spur branch ends and confirm the new support-node encounter weight feels different by frontage type, with softer residential pull and stronger market or service pressure instead of every branch end carrying the same pacing.
- Move on foot and by vehicle through the widened outer branches and confirm those aligned support-node pulls make the side routes feel more like lightly staged local streets than one darker branch strip hanging off the arterial skeleton.
- Visit several widened outer branch ends and confirm each one now carries a shallow frontage apron plus a small wall or canopy mass on the support side instead of falling back to darker branch asphalt with only loose furniture around the marker.
- Compare residential, transit-market, civic-retail, and service-spur branch edges and confirm the new branch-side massing changes by frontage feel, with calmer residential sheltering, broader market canopies, heavier service loading edges, and tighter civic-retail kiosk structure instead of one repeated branch-side slab.
- Approach widened branch ends on foot and by vehicle and confirm the deeper branch vehicle handoff now stages off the same support-node frontage geometry and nearby sidewalk pull point instead of floating at a separate generic branch offset.
- Visit several widened outer branch ends and confirm each one now carries a deeper parcel pocket or edge breakup behind the support-side massing instead of stopping at the earlier apron and canopy layer.
- Compare residential, transit-market, civic-retail, and service-spur branch pockets and confirm the new parcel-edge language changes by frontage feel, with calmer fences and trees, market queue pads, heavier service screens, and tighter civic-retail plinths instead of one repeated rear-lot strip.
- Approach widened branch ends on foot and by vehicle and confirm the new parked-vehicle pocket and nearby civilian cluster now stage off that same support-side parcel pocket instead of floating as separate simulation hooks.
- Visit several widened outer branch ends and confirm each one now carries a second rear continuity pad or small landmark structure behind the parcel pocket instead of letting the new branch-side lot depth stop abruptly at the first pocket layer.
- Compare residential, transit-market, civic-retail, and service-spur branch pockets and confirm the new rear continuity layer changes by frontage feel, with calmer residential carport rhythm, taller market kiosks, heavier service bay screens, and tighter civic-retail marker structures instead of one repeated rear pocket cap.
- Move on foot and by vehicle through widened outer branches and confirm the deeper continuity hotspot, pedestrian pull, extra parked staging, and passive stop-zone pressure now cluster around that same rear continuity structure instead of leaving the new edge layer visually present but systemically quiet.
- Visit several widened outer branch ends and confirm each one now carries a deeper secondary frontage massing or rear-lot shelter behind the continuity pad instead of letting the branch silhouette stop at the rear cap structure.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new secondary frontage layer changes by frontage feel, with calmer residential carport shelter, taller market kiosk panels, heavier service loading screens, and tighter civic-retail rear bays instead of one repeated secondary slab.
- Move on foot and by vehicle through widened outer branches and confirm the deeper secondary hotspot, extra civilian clustering, added parked staging, and another stop-zone hesitation beat now sit on that same secondary frontage geometry instead of making the new rear massing visually present but systemically inert.
- Visit several widened outer branch ends and confirm each one now carries a deeper rear-edge wall, bay, or parcel-edge structure behind the secondary frontage instead of letting the local-street silhouette stop at the secondary shelter.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new rear-edge layer changes by frontage feel, with calmer residential rear walls, taller market sign panels, heavier service yard edges, and tighter civic-retail back bays instead of one repeated rear-edge slab.
- Move on foot and by vehicle through widened outer branches and confirm the deeper rear-edge hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same rear-edge structure instead of making the new wall layer visually present but systemically quiet.
- Visit several widened outer branch ends and confirm each one now carries a deeper back-lot wall, canopy span, or service-yard cover behind the rear-edge structure instead of letting the branch fabric stop at the parcel-edge wall.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new back-lot coverage layer changes by frontage feel, with calmer residential garage or trellis spans, taller market back-canopy screens, heavier service-yard sheds, and tighter civic-retail backroom bays instead of one repeated far-edge slab.
- Move on foot and by vehicle through widened outer branches and confirm the deeper back-lot hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same back-lot coverage geometry instead of making the far branch layer visually present but systemically quiet.
- Visit several widened outer branch ends and confirm each one now carries a deeper infill wall, courtyard closure, or service-yard return behind the back-lot coverage instead of letting the branch fabric stop at the far canopy or shed line.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new infill layer changes by frontage feel, with calmer residential garage returns, taller market infill panels, heavier service-yard wall returns, and tighter civic courtyard closures instead of one repeated inner back-lot slab.
- Move on foot and by vehicle through widened outer branches and confirm the deeper infill hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same infill geometry instead of making the inner closure layer visually present but systemically quiet.
- Visit several widened outer branch ends and confirm each one now carries a deeper inner enclosure, rear-building silhouette, or service-yard interior structure behind the infill layer instead of letting the branch fabric stop at the first courtyard closure.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new inner-enclosure layer changes by frontage feel, with calmer residential rear-building returns, taller market inner readout panels, heavier service-yard interior partitions, and tighter civic interior court walls instead of one repeated inner closure slab.
- Move on foot and by vehicle through widened outer branches and confirm the deeper inner-enclosure hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same inner-enclosure geometry instead of making the deeper closure layer visually present but systemically quiet.
- Visit several widened outer branch ends and confirm each one now carries a deeper core backstop, rear-core silhouette, or service-yard interior closure behind the inner-enclosure layer instead of letting the branch fabric stop at the first inner court wall.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new core-backstop layer changes by frontage feel, with calmer residential rear-core stubs, taller market inner readout cores, heavier service-yard interior closures, and tighter civic rear-court backstops instead of one repeated deepest branch slab.
- Move on foot and by vehicle through widened outer branches and confirm the deeper core-backstop hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same core-backstop geometry instead of making the deepest branch layer visually present but systemically quiet.
- Visit several widened outer branch ends and confirm each one now carries a deeper rear-core massing, court interior closure, or service-yard backstop structure behind the core-backstop layer instead of letting the branch fabric stop at the final backstop wall.
- Compare residential, transit-market, civic-retail, and service-spur branch depths and confirm the new rear-core layer changes by frontage feel, with calmer residential garden-wall or court-nook massing, taller market readout cores, heavier service-yard backstop walls, and tighter civic rear-court closures instead of one repeated deepest branch cap.
- Move on foot and by vehicle through widened outer branches and confirm the deeper rear-core hotspot, extra pedestrian pull, added parked staging, and another hesitation beat now sit on that same rear-core geometry instead of making the deepest massing layer visually present but systemically quiet.
- Treat this whole pass as the first post-milestone faction check: traversal, staged vehicle escape, lane combat, civilian spillover, traffic easing, normalization, reopening, territory crossing, and hot re-entry should now read as one socially structured graybox slice instead of a neutral sandbox.

The current prototype is a macOS graybox sixty-four-block Los Angeles reference grid with nine authored streaming tiles, sixteen named corridor spines, generated cross-city road links, broader district metadata, district-weighted population profiles, shared road-spine descriptors plus road-class cross-section profiles that now drive road surfaces, curb width, sidewalk depth, lane markings, lane pull, and ground queries from the same metadata, explicit corridor and world-chunk descriptor tables that now steer intersection spacing and block-edge treatment, a frontage descriptor layer that now blends frontage template with corridor and chunk bias for shopfront setback, furniture lane spacing, transit-stop stand-off, rear fence depth, awning span, and service loading-zone placement, boulevard-versus-avenue street treatment, intersection-aware crosswalk or stop-bar spacing, corner plaza pads, signal setbacks, and boulevard refuge islands, clearer service-lane loading-zone readouts in the spur blocks, frontage-template-driven block variants, hotspot hooks, engine-exported traffic occupancy hooks, route-aware placeholder traffic with stop-zone holds and early near-miss hazard pulses, staged sedan, coupe, moped, bicycle, and motorcycle handoffs with ranked selection cues and lock controls, richer non-player civilian and patrol silhouettes with layered shoulders, torsos, arms, legs, necks, and heads instead of simple stacked-box mannequins, a broader vehicle form pass that now gives staged, ambient, and parked traffic more believable hood, cabin, roof, and rounded-wheel layering rather than two-box car bodies, and now a static streetscape massing pass that adds parapets, plinths, glazing bands, corner trim, rooftop equipment, denser transit shelters, fuller benches, layered planters, richer bollards, and stronger corner-store frontage detail so the street edge no longer falls back to plain box props as often. That same shared streetscape layer now also gives storefront awnings deeper valances, glazing bands, end caps, and support rods, upgrades signal poles with bases, mast arms, and readable signal heads, rebuilds newsstands, trash bins, and apartment entries with framed faces, lids, wheel housings, stoops, and canopy or door detail, carries a shared back-lot realism pass with utility-pole runs, overhead wire spans, asphalt side-lot pads, curb edges, stall striping, wheel stops, and planter islands so the outer block wings stop collapsing back to plain lot fill, and now adds storefront kickplates, recessed entries, transom bands, clearer mullions, plus worn parking-pad surfaces with darker tire tracks, repair patches, faded striping, and oil marks so the city reads as more materially used-in instead of freshly assembled. The curbside layer now also threads hydrants, parking meters, signal-control boxes, utility cabinets, and separate bus-stop signs through the shared shelter, intersection, frontage, and service-court builders so the widened slice starts reading like a serviced city street instead of a clean procedural curb, and the roadway itself now carries darker gutter bands, repeated storm drains, and lane service covers along the shared boulevard and avenue spines so the corridor stops reading as one flat asphalt slab between curbs. The intersection-mouth pass now also adds stepped curb-return geometry outside the crosswalks plus short lane-mouth transition patches on every shared approach so corridor entries stop reading as perfect square grid cutouts when they meet the node, the node-center pass now layers brighter crosswalk landing pads, darker center-surface breakup, and small interior wear patches into the shared intersections so they stop collapsing back into one flat repeated asphalt box, the boulevard refuge pass now gives the shared median islands capped ends, center seams, and short crosswalk-adjacent stripe clusters so those approaches feel more infrastructurally finished instead of like plain planter boxes dropped into the asphalt, the signalized node-edge pass now frames each crosswalk with darker edge skirts and short end caps, adds small signal-corner wait pads on the sidewalk, and lays darker throat patches plus guide marks between boulevard refuges and the crosswalk zone so the approaches read more like built signal hardware than painted boxes on a flat slab, the stop-bar or arrow support pass now gives each shared stop bar a darker shoulder apron with end structure and each lane arrow a darker stand-off pad with seam and side guides so those control marks stop floating as isolated symbols inside the node, the boulevard divider-throat pass now carries paired darker strips and painted caps between the stop-bar and arrow zones so the boulevard control language stays continuous through the approach instead of dropping out between the median-divider marks and the rest of the node kit, the signal-corner service pass now gives each controller corner a darker cabinet pad, conduit-style surface runs, a small pull-box cover, and a hydrant footing so the hardware reads like serviced street infrastructure instead of props sitting on clean plaza fill, the turn-pocket surface pass now lays a darker, slightly flared pocket between each stop-bar shoulder and arrow stand-off so the approach controls read as one continuous lane-management surface instead of separate stamps on flat asphalt, the service-edge wear pass now darkens the cabinet-pad borders, broadens the conduit scuff bands, and gives each hydrant footing a heavier worn surround so those controller corners stop reading as freshly cut shapes in untouched sidewalk, the pedestrian-corner pass now adds tactile warning pads, short approach bands, and curb-ramp seam scoring so the signalized wait zones stop reading as plain bright rectangles dropped at the curb, the pole-corner hardware pass now gives each signal pole a darker scored footing apron plus paired pedestrian call boxes aligned to the two crossing directions so the controlled corners stop reading like bare mast poles dropped into the sidewalk, the mast-detail pass now adds visible hangers, top clamps, and rear head brackets so the signal heads stop reading like clean boxes simply floating under the arm, the reinforcement pass now adds mast collars plus short arm-to-pole braces so the horizontal arm reads like a supported assembly instead of a clean beam plugged straight into the post, the pedestrian-readout pass now gives each inward-facing pole pair small crossing displays with distinct upper and lower readout bands so the corners stop reading like button hardware alone, the mast-wiring pass now adds a darker secondary cable trunk with small clamps plus short drops to the center and outer heads so the mast no longer reads electrically empty, the signal-head finish pass now adds short visors plus a lighter upper face fade and darker lower grime band so those heads stop reading like bare flat-color boxes, the rear-housing pass now adds subtle seam bands, a rear access panel, and a small lower latch so the back of each signal head stops reading like one clean slab, the side-detail pass now adds a slim hinge spine with small upper and lower caps so the head profile stops collapsing into a clean box edge at oblique angles, the rear-coupling pass now adds a small conduit stub with a brighter collar near the upper back so the service entry stops feeling implicit, the lower-lip pass now adds a small drain edge with a brighter forward lip and a darker rear outlet so the underside stops reading like a clean flat cutoff, the mount gasket pass now adds a small compressed seal pad plus narrow side tabs where the mount meets the head so the connection stops reading like a clean direct join, the backplate-shim pass now adds slim side and top spacers behind the housing so the backplate reads as a layered offset assembly instead of a clean slab, the mount-clamp seam pass now adds small center seams plus end-cap breakup to the upper and lower clamp blocks so the mount stops reading like one untouched piece, the corner-fastener pass now adds small housing-corner fastener hints so the head stops reading like a perfectly clean cast box, the visor-side attachment pass now adds small side straps and caps so the visor stops reading like one uninterrupted folded shade, the side service-tab pass now adds a restrained access-panel hint and tiny latch nubs so the body side no longer reads perfectly smooth, and the new lens-baffle spacer pass now adds subtle horizontal spacer bars plus side rails around the signal lenses so the head face stops collapsing into one open panel. The combat scaffold still includes a lead-pipe pickup, a pistol pickup, weapon-slot switching, ammo and reload state, muzzle and impact feedback, cover-aware firearm blocking, a lookout hostile that can return fire, pressure-anchor-driven hostile repositioning, two authored civilian reaction sources around the lane, ambient pedestrians that can peel away from live incidents, a lightweight hostile last-seen search state, a persistent street-incident layer that spills into the nearest adjacent road links, a distinct street-reopening handoff after normalization, warmer spillover and cooler normalization/reopening ambient-link feedback, and the first faction-territory scaffolding with a readable court-set boundary at the lane mouth, claimed-turf readouts inside the block, a visible court-set sidewalk runner, a second ambient inner post replacing the old deep marker, simple runner-to-post handoff rules, a clearer sidewalk screening beat before full territory commitment, a brief complementary clamp at the boundary when the player hesitates, a lightweight clear-line beat once the inner post or lookout fully owns pressure, a lightweight reform/re-approach beat when the player backs out, a lightweight harden/brace beat when the player recommits during reform, a lightweight lane-mouth commit window once that hardened line is live, a short push-through objective progress and completion beat without a separate mission panel, a tiny follow-through resolve after the line break where the player can either briefly hold the pocket or pull out cleanly, distinct short aftermaths for those two outcomes, a short next-pass memory after each one, a slightly richer edge texture on those returns where held pockets can show a softer reclaim feint before the deeper pocket commits and clean pullouts can throw an earlier edge challenge before recommitting through the mouth, a more active repeated-half-commit answer on top of that texture so shallow recommits and backing-off loops can keep the reclaim side baiting deeper pressure while the retake side hardens the mouth check sooner, a small positional answer on top of those edge beats so the runner can counter-step off the softer mouth while the inner post turns the deeper angle and the retake side can cross-angle the lane mouth before the heavier recommit lands, and now a lightweight remembered shoulder on top of that so repeated left-right approaches can bias which side of the mouth the runner and post answer through on reclaim and retake returns, with that same shoulder now carrying farther through deeper reclaim handoffs and edge reseals instead of flattening back into a neutral follow-through, leaning the lookout anchor plus the broader street-search response toward that same shoulder during held-pocket reclaim and clean-pullout reseal, staying readable through normalization, reopening, and hot re-entry instead of dropping that shoulder memory as soon as the lane cools, pulling reopening spillback plus runner-post boundary reform toward that same shoulder so the block does not reopen from a generic center, leaving a softer same-shoulder late fallback behind after reopening fades so the next quiet line watch does not reset from a neutral default, carrying that remembered shoulder into the first fully cold screen or clamp so the next fresh boundary catch does not snap back to center either, letting that same carry survive one beat earlier into the first neutral outside watch before the colder screen or clamp forms, and now letting it survive one beat earlier again into the runner’s first far-out drift off idle before that outside watch fully forms. The in-scene combat HUD now carries both that territory state and a dedicated map card with district, tile, and corridor readouts alongside the street-system card, so player health, recovery, systemic pressure, faction presence, sidewalk watch, entry screen, entry clamp, clear-line handoff, reform, hardening, commit window, line break, hold-versus-pullout resolve, pocket reclaim, edge retake, reclaim return, reclaim feint, reclaim commit, retake return, edge challenge, retake commit, territory crossing, hostile escalation, and named world context can be read together instead of as unrelated debug fragments. Use the on-screen overlay and the floating controls window together for controls, chunk visibility, staged vehicle prompts, combat state, street pressure, traffic reaction, civilian reaction, territory heat, watch, patrol alert, inner-post alert, handoff ranking and lock state, grip, lane, bump, recovery, hazard feedback, and active block metadata.

The latest node-control pass now also gives each shared signal head a small rear vent frame, dark slots, and a tiny rain lip so the upper back reads as serviced housing instead of one uninterrupted slab.

The new visor pass now also gives each shared signal head tiny visor-cap joint strips plus a small center cap plate so the top shade reads as assembled hardware instead of one uninterrupted folded slab.

The latest rear-housing pass now also gives each shared signal head a small rear-panel fastener cluster so the service panel reads as maintained hardware instead of one clean stamped piece.

The latest visor pass now also gives each shared signal head a dark underside join strip plus tiny side tie-ins so the shade reads more like folded sheet hardware instead of one clean slab from below.

The latest rear-service pass now also gives each shared signal head a small clamp band with tiny side tabs around the rear coupling so the service entry reads more like an assembled hold-down instead of one smooth conduit collar.

The latest underside pass now also gives each shared signal head a dark seam line plus tiny end caps along the lower drain lip so the edge reads more like a folded strip instead of one clean underside bar.

The latest mount pass now also gives each shared signal head a darker brace edge plus tiny brace caps so the support reads more like fabricated plate work instead of two smooth brace blocks.

The latest mast pass now also gives each shared head hanger a dark seam strip plus a tiny upper collar so the drop bars read more like fabricated hanger hardware instead of smooth arm posts.

The latest arm pass now also gives each shared reinforcement collar a darker edge band plus a tiny top cap so the support reads more like fabricated collar hardware instead of smooth support blocks.

The latest wiring pass now also gives each shared arm clamp a darker seam band plus a tiny top cap so the cable hold-downs read more like strapped clamp hardware instead of smooth clamp blocks.

The latest pedestrian-signal pass now also gives each shared readout mount a darker seam band plus a tiny center cap so the rear mount reads more like fabricated hardware instead of one smooth support block.

The latest map-foundation pass now also expands the engine into an eight-by-eight Los Angeles corridor grid with generated road links, generated population profiles, nine named streaming tiles, and sixteen named roads so the app can start carrying the full gang-map footprint as a connected skeleton instead of a premium core surrounded by missing space.

The latest connector pass now also adds shared frontage or service side lanes around the widened block fabric, teaches ground classification to treat those lanes as real road space, and spreads extra hotspot plus staged-vehicle hooks into the outer tiles so the enlarged Los Angeles footprint starts feeling systemically inhabited instead of only geometrically present.

The latest route-variation pass now also breaks those outer connector lanes into district-specific branch or cut-through patterns, pushes landmark and hotspot hooks deeper along those branches, and feeds light stop-zone pressure through the new route nodes so the widened Los Angeles footprint starts reading more like a layered street network than one repeated arterial-plus-frontage template.

The latest route-support pass now also gives those widened branch ends visible route markers, frontage-side support props, and deeper branch-biased vehicle or pedestrian handoff anchors so the outer Los Angeles routes start reading like physically identified traversal choices instead of passive side geometry.

The latest branch-activity pass now also clusters extra hotspot and pedestrian pull points around those marked branch nodes, adds frontage-specific dynamic route signaling at the support ends, and extends passive stop-zone pressure onto the support nodes themselves so the widened outer routes start reading like light encounter destinations instead of only traversal stubs.

The latest branch-context pass now also reuses one shared outer-route node context across the visible support kit, hotspot clustering, and passive stop-zone weighting so the widened branches keep their structural and encounter beats spatially aligned instead of drifting apart as the outer network grows.

The latest branch-frontage pass now also layers frontage-specific aprons, small support-side wall or canopy massing, and support-aligned branch vehicle staging onto that same shared outer-route node context so the widened branches read more like local street edges with places to stop instead of darker side lanes with disconnected props.

The latest branch-parcel pass now also adds frontage-specific pocket lots, edge screens, and civilian or parked-vehicle pocket staging behind those support nodes so the widened outer branches carry more believable parcel depth and low-intensity local activity instead of ending at the first canopy and marker layer.

The latest branch-continuity pass now also extends those parcel pockets into frontage-specific rear pads and small landmark structures, adds deeper hotspot or pedestrian pull plus extra parked staging around that same geometry, and feeds one more layer of passive stop-zone pressure through those deeper branch pockets so the widened outer routes read more like continuous local streets instead of parcel pockets that stop at the first support kit.

The latest branch-secondary-frontage pass now also adds a deeper shared rear-frontage shelter or bay behind those continuity pads, clusters extra civilian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the deeper branch pocket so the widened outer routes read more like continuous parcel-aware local streets instead of rear pads capped by one final marker structure.
The latest branch-rear-edge pass now also adds a deeper shared parcel-edge wall or bay behind the secondary frontage, clusters one more pedestrian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the rear branch pocket so the widened outer routes read more like continuous built street edges instead of rear pads capped by a single shelter layer.
The latest branch-backlot-coverage pass now also adds a deeper shared back-lot wall, canopy, or service-yard span behind the rear-edge layer, clusters one more pedestrian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the far branch pocket so the widened outer routes read more like denser local street fabric instead of parcel-edge walls capped by one final back line.
The latest branch-infill pass now also adds a deeper shared infill wall, courtyard closure, or service-yard return behind the back-lot layer, clusters one more pedestrian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the inner branch pocket so the widened outer routes read more like denser enclosed local street fabric instead of back-lot coverage ending at a single far span.
The latest branch-inner-enclosure pass now also adds a deeper shared rear-building silhouette, court wall, or service-yard interior partition behind the infill layer, clusters one more pedestrian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the deepest branch pocket so the widened outer routes read more like denser enclosed local street fabric instead of infill closures ending at a single inner wall.
The latest branch-core-backstop pass now also adds a deeper shared rear-core backstop, inner court wall, or service-yard interior closure behind the inner-enclosure layer, clusters one more pedestrian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the deepest branch pocket so the widened outer routes read more like denser enclosed local street fabric instead of inner-enclosure walls ending at a single final backstop.
The latest branch-rear-core pass now also adds a deeper shared rear-core massing, court interior closure, or service-yard backstop structure behind the core-backstop layer, clusters one more pedestrian pull and another parked-stage anchor around that same geometry, and feeds one more hesitation layer through the deepest branch pocket so the widened outer routes read more like denser enclosed local street fabric instead of core-backstop closures ending at a single final cap.

### Any questions ###

Contact barbalet at gmail dot com
