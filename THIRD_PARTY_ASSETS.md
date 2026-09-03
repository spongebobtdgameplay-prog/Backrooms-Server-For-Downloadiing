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

## Procedural audio

- Continuous fluorescent hum is synthesized at runtime with WebAudio oscillators.
- Continuous static/noise music is synthesized at runtime with a generated noise buffer and filters.
- Shapeshift static bursts are synthesized at runtime.
- These procedural audio layers do not contain third-party recordings.

## Internal shared code

- `src/CollisionUtility.js`
  - Synchronized from `spongebobtdgameplay-prog/The-Infinity-Store/collision-utility.js`.
  - Source build marker: `V0.35.39-CACHED-FURNITURE-MOVEMENT`.
  - Shared project code, not a third-party asset.
