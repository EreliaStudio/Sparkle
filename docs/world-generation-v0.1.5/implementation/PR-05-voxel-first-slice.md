# v0.1.5 — PR05: First end-to-end headless voxel generation slice

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Voxel Core. **Dependencies:** PR02–PR04.

## Objective

Prove the architecture can convert a generic, deterministic world plan into neighboring Voxel::Volume snapshots with a useful visible logical pattern.

## Proposed scope

- Implement the smallest voxel realization adapter using existing Voxel::Volume::Builder and Definition::Registry.
- Accept requested chunk coordinates and dimensions, recipe/plan and registry; build one immutable Volume.
- Generate a simple continuous ground pattern from absolute world coordinates.
- Query the plan for applicable features; initially one deterministic sample descriptor is enough.
- Publish through generic chunk collection; do not introduce alternate voxel cell storage.
- Use no OpenGL and no new network messaging (the existing Volume stream API is sufficient).

## Required automated tests

- Two neighboring chunks line up at the boundary for several seeds and negative coordinates.
- Exact equality after reordering requests and after regeneration.
- Correct known Definition IDs, air/ground transitions, builder construction and invalid inputs.

## Explicit exclusions

No biome catalog, roads/cities/caves, streaming, background threads or golden images.

## Architecture review before coding

Confirm voxel adapter naming and parameters after reviewing a concrete two-chunk example.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
