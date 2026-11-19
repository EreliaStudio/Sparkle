# PR05 — ChunkInterest and registered WorldObserver

**Layer:** Core/EngineHeadless. **Dependencies:** PR01. **Status:** design draft.

## Scope
- `ChunkInterest` holds a nonnegative `ChunkCoordinate` **radius** (one value per XYZ axis), defining an inclusive axis-aligned box around a center chunk.
- `WorldObserver : Component3D` is an opt-in marker/configuration for player or camera entities; it carries the interest and uses its Entity3D world-space transform.
- Automatic discovery through existing `Registry<WorldObserver,EngineHeadless*>`, via existing Component3D context propagation; **no new registration mechanism**.
- One world/collection per Engine v0.1.5; later world UUID filtering deliberately deferred.
- Streamer will compute center using the chunk type's static world-to-chunk conversion; ChunkInterest does not compute rendering priorities.

## Required tests
- Component follows owner/parent Engine context, registers on attachment and is removed on detach/destruction.
- Only entities bearing WorldObserver contribute; monsters without the component do not.
- Radius (4,2,4) yields 9×5×9 = 405 candidate coordinates; correct at negative centers and near coordinate limits.
- Multiple observers in EngineHeadless without Graphics.

## Exclusions
No frustum visibility query, flood fill, separate world context, UUID or streamer implementation.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
