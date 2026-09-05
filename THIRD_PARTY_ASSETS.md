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

- `assets/audio/entity-death.ogg`
  - Creator: CVLTIV8R
  - Original source: Freesound
  - Source game credit lists this asset as CC0.
  - Vendored from the public fever-channels repository.

- `assets/audio/entity-laugh.ogg`
  - Creator: Nanakisan
  - Original source: Freesound sound 253534
  - License used here: Creative Commons Attribution 4.0
  - Attribution: "Evil Laugh 9.wav" by Nanakisan, Freesound.
  - Vendored as an OGG from the public fever-channels repository.
  - The direct Freesound page currently reports Attribution 4.0, so this repository follows that stricter license.

## Legacy procedural audio

- Older generated WAV layers remain in the repository as fallback compatibility assets.
- V0.3.23 and later use the recorded CC0 fluorescent hum and Kenney interaction/footstep recordings above as the primary native soundscape.

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
  - The native renderer uses the real mesh geometry and a neutral metal base color; external texture files are not required by this build.

## Native environment and interaction audio

- `assets/audio/fluorescent-hum.ogg`
  - Creator: ftpalad
  - Original source: Freesound sound 119910, `Fluorescent Lightbulb Hum.aif`
  - License: CC0 1.0
  - Vendored from a public GitHub copy of the Freesound asset.

- `assets/audio/footstep-carpet-1.ogg` through `footstep-carpet-4.ogg`
  - Creator: Kenney
  - Original pack: Kenney RPG Audio (`cloth1.ogg` through `cloth4.ogg`)
  - License: CC0 1.0
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.

- `assets/audio/breaker-trip.ogg`
  - Creator: Kenney
  - Original pack: Kenney Interface Sounds (`switch_003.ogg`)
  - License: CC0 1.0
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.

- `assets/audio/entity-metal.ogg`
  - Creator: Kenney
  - Original pack: Kenney RPG Audio (`metalPot1.ogg`)
  - License: CC0 1.0
  - Used as a quiet positional distant-metal cue for the entity encounter.
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.
