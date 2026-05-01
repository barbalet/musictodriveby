# Art Reset Inventory

This pass replaces the most obvious graybox stand-ins with downloaded meshes while keeping the existing C gameplay simulation, collision, and procedural debug overlays intact.

## Downloaded Free Sources

- [Kenney Blocky Characters](https://www.kenney.nl/assets/blocky-characters)
  - Local archive: `Assets/downloads/kenney_blocky-characters.zip`
  - Status: downloaded and applied
  - License: page lists Creative Commons CC0
- [Kenney Car Kit](https://kenney.nl/assets/car-kit)
  - Local archive: `Assets/downloads/kenney_car-kit.zip`
  - Status: downloaded and applied
  - License: page lists Creative Commons CC0
- [Kenney City Kit Roads](https://www.kenney.nl/assets/city-kit-roads)
  - Local archive: `Assets/downloads/kenney_city-kit-roads.zip`
  - Status: downloaded and applied
  - License: Kenney asset page family is CC0
- [Kenney City Kit Commercial](https://kenney.nl/assets/city-kit-commercial)
  - Local archive: `Assets/downloads/kenney_city-kit-commercial.zip`
  - Status: downloaded and applied
  - License: Kenney asset page family is CC0
- [Kenney City Kit Suburban](https://www.kenney.nl/assets/city-kit-suburban)
  - Local archive: `Assets/downloads/kenney_city-kit-suburban.zip`
  - Status: downloaded and applied
  - License: Kenney asset page family is CC0
- [Kenney City Kit Industrial](https://kenney.nl/assets/city-kit-industrial)
  - Local archive: `Assets/downloads/kenney_city-kit-industrial.zip`
  - Status: downloaded and applied
  - License: Kenney asset page family is CC0
- [Quaternius Animated Guns Pack](https://quaternius.com/packs/animatedguns.html)
  - Local files: `Assets/downloads/quaternius_pistol.obj`, `Assets/downloads/quaternius_pistol.mtl`
  - Status: downloaded and applied for firearm visuals
  - License: page lists CC0

## Component Inventory

- Humans
  - Player avatar
  - Ambient pedestrians
  - Witness
  - Bystander
  - Combat dummy
  - Hostile lookout
  - Territory patrol actor
  - Deep territory watcher / presence actor
- Weapons
  - Firearm pickup
  - Equipped firearm
  - Lead pipe pickup
  - Equipped lead pipe
- Vehicles
  - Sedan
  - Coupe
  - Moped
  - Bicycle
  - Motorcycle
- Environment art
  - Residential houses
  - Commercial buildings and awnings
  - Industrial buildings, tanks, chimneys
  - Trees
  - Fences
  - Planters
  - Street lights
  - Road signs
  - Cones
- Procedural-only gameplay visuals that intentionally remain non-mesh
  - Health bars
  - Focus markers
  - Hit flashes
  - Search / incident / territory pulses
  - Pickup halos and target rings

## Current Demo Mapping

- Humans
  - Applied source: Kenney Blocky Characters
  - In-game use: player, pedestrians, witness/bystander, combat dummy, hostile, territory patrol, and territory watchers now render as character meshes instead of stacked boxes.
- Weapons
  - Applied source: Quaternius Animated Guns Pack `Pistol.obj/.mtl`
  - In-game use: firearm pickup, player sidearm, and hostile sidearm now use a real pistol mesh instead of the old boxy stand-in.
  - Current gap: the lead pipe is still procedural in this pass and should be replaced next with a dedicated CC0 tool / melee prop mesh.
- Vehicles
  - Applied source: Kenney Car Kit
  - In-game use: sedan and coupe read directly as cars; smaller fallback meshes are also used for the current two-wheel traversal slots so every vehicle state now renders with an imported model rather than pure boxes.
  - Current gap: bicycle, moped, and motorcycle still need one-to-one dedicated meshes instead of nearest-fit fallback vehicles.
- Environment
  - Applied sources: Kenney City Kit Roads, Commercial, Suburban, Industrial
  - In-game use: visible block-owned environment geometry is replaced with imported buildings and props while the shared road / ground substrate stays procedural for simulation continuity.

## Playable Demo Scope

- The renderer now supports bundled OBJ + MTL content, atlas textures, and plain MTL diffuse-color materials.
- A curated resource set lives under `Sources/MusicToDriveBy/Resources/ArtReset`.
- `swift build` passes with the imported meshes wired into the live renderer.
- `swift run MusicToDriveBy` launches as a smoke test for the current demo pass.

## Next Art Targets

- Replace the procedural lead pipe with a dedicated CC0 melee prop.
- Replace the temporary bicycle / moped / motorcycle fallback meshes with dedicated two-wheel models.
- Upgrade the current human source from readable stylized figures to a more realistic character pack if the project wants to push harder toward GTA-like visual targets.
