# Third-Party Assets

## Entity models

- `assets/models/entity-ghost.glb`
  - Source: Quaternius, Ultimate Monsters
  - License: CC0 1.0
  - The Ultimate Monsters pack is distributed by Quaternius as CC0 and includes animated glTF monsters.
  - Vendored from the public warptracker repository, whose asset manifest records this model as Quaternius Ultimate Monsters.

- `assets/models/entity-demon.glb`
  - Source: Quaternius, Ultimate Monsters
  - License: CC0 1.0
  - The Ultimate Monsters pack is distributed by Quaternius as CC0 and includes animated glTF monsters.
  - Vendored from the public warptracker repository, whose asset manifest records this model as Quaternius Ultimate Monsters.

## Entity audio

- `assets/audio/entity-death.wav`
  - Creator: CVLTIV8R
  - Original source: Freesound
  - Source game credit lists this asset as CC0.
  - Vendored from the public fever-channels repository.

- `assets/audio/entity-laugh.wav`
  - Creator: Nanakisan
  - Original source: Freesound sound 253534
  - License used here: Creative Commons Attribution 4.0
  - Attribution: "Evil Laugh 9.wav" by Nanakisan, Freesound.
  - Vendored from the public fever-channels repository and converted to PCM WAV for the native build.
  - The direct Freesound page currently reports Attribution 4.0, so this repository follows that stricter license.

## Legacy procedural audio

- Older generated WAV layers remain in the repository as fallback compatibility assets.
- V0.3.24 and later use PCM WAV conversions of the recorded fluorescent hum and Kenney interaction/footstep recordings above as the primary native soundscape.

## Internal shared code

- `src/CollisionUtility.js`
  - Synchronized from `spongebobtdgameplay-prog/The-Infinity-Store/collision-utility.js`.
  - Source build marker: `V0.35.39-CACHED-FURNITURE-MOVEMENT`.
  - Shared project code, not a third-party asset.


## Breaker model

- `assets/models/power_box_01/power_box_01_1k.gltf`
- `assets/models/power_box_01/power_box_01.bin`
  - Asset: Power Box 01
  - Creators: Rico Cilliers and Yann Kervran
  - Original source: Poly Haven
  - License: CC0 1.0
  - Vendored from the public `lanathlor/pyrrhic-stars` mirror of the Poly Haven model.
  - V0.3.25 ships the matching 1K diffuse, normal, and ARM texture files and teaches the native glTF loader to resolve external image URIs. The breaker door rotation is reduced so the switches and wiring remain readable from the hallway.

## Native environment and interaction audio

- `assets/audio/fluorescent-hum.wav`
  - Creator: ftpalad
  - Original source: Freesound sound 119910, `Fluorescent Lightbulb Hum.aif`
  - License: CC0 1.0
  - Vendored from a public GitHub copy of the Freesound asset.

- `assets/audio/footstep-carpet-1.wav` through `footstep-carpet-4.wav`
  - Creator: Kenney
  - Original pack: Kenney RPG Audio (`cloth1.wav` through `cloth4.wav`)
  - License: CC0 1.0
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.

- `assets/audio/breaker-trip.wav`
  - Creator: Kenney
  - Original pack: Kenney Interface Sounds (`switch_003.wav`)
  - License: CC0 1.0
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.

- `assets/audio/entity-metal.wav`
  - Creator: Kenney
  - Original pack: Kenney RPG Audio (`metalPot1.wav`)
  - License: CC0 1.0
  - Used as a quiet positional distant-metal cue for the entity encounter.
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.


## Native audio packaging

- V0.3.24 converts the credited recorded source files to mono 44.1 kHz PCM WAV at build-preparation time so the shipped native game does not depend on optional OGG/Vorbis decoder support.


## Exit door model

- `assets/models/exit-door.glb`
  - Source: `spongebobtdgameplay-prog/The-Infinity-Store`, `Models/Architecture/GLB/Door_3.glb`.
  - Usage: authored Level 0 exit-door mesh replacing the normal procedural box-built exit visual.
  - This is shared project content from the user's Infinity Store repository.
