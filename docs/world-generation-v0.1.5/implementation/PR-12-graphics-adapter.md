# v0.1.5 — PR12: Voxel chunk meshing and renderer integration

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Graphics. **Dependencies:** PR05, PR11.

## Objective

Consume existing generated Volume snapshots and render them through Sparkle's current voxel mesher/rendering pipeline.

## Proposed scope

- Associate loaded chunk coordinates with Engine entities/transforms only in Graphics.
- Use existing Voxel::Mesher and its outsideCell hook for neighboring chunks.
- Rebuild current geometry on chunk revision, adjacent-face change or neighbor unload.
- Release geometry/entities cleanly when chunks leave the interest set.
- Allow future non-voxel visualization adapters without changing generic Core.

## Required automated tests

- Goldens for adjacent surfaces without seams, unloaded/unknown neighbors, geometry replacement.
- Chunk transformations including negative offsets, larger destinations and varied geometry.
- Verify exact invalidated chunk/neighbor sets and resource lifetimes using built-in Sparkle test utilities.

## Explicit exclusions

No new global rendering engine, no GPU handling in Core, no golden skip and no Python comparator.

## Architecture review before coding

Decide whether a generic rendering adapter abstraction adds value beyond the first concrete voxel renderer integration.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
