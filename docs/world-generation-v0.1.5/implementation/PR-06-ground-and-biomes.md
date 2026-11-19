# v0.1.5 — PR06: Terrain sampling, ground stratification and biome metadata

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core + Voxel Core. **Dependencies:** PR03–PR05.

## Objective

Offer generic spatial scalar fields and a voxel ground recipe without locking Sparkle to one terrain style or noise library.

## Proposed scope

- Provide configurable field/noise sampling with stable seed and continuous world coordinates.
- Add typed descriptors for elevation, humidity, temperature, biome weights and region-level constraints.
- Support layered terrain material selection through existing Definition IDs.
- Permit quantized/terraced altitude, explicit slopes or staircase placement as optional voxel-specific policies.
- Ensure biome changes and ground seams remain consistent across chunk and region boundaries.

## Required automated tests

- Constant, planar, stepped, sloped and multi-octave fields with exact boundary samples.
- Biome changes, clamped or out-of-range settings, deterministic materials and no missing palette IDs.
- Different seeds produce different layouts; repeated seeds match bitwise at defined quantized outputs.

## Explicit exclusions

No mandatory biomes for all world types, no weather simulation, no dynamic erosion and no Graphics coupling.

## Architecture review before coding

Agree on expected precision across platforms and whether biome blending is metadata-only or a reusable default algorithm.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
