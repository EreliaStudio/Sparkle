# v0.1.5 — PR04: World/region plans, metadata and spatial feature queries

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core. **Dependencies:** PR01, PR03.

## Objective

Introduce lightweight typed planning metadata so coherent multi-chunk features can exist independent of the chunks that will later realize them.

## Proposed scope

- Support an immutable world plan and on-demand region plans, with explicit bounded world-level inputs.
- Define spatial feature descriptors with stable IDs, bounds and typed metadata.
- Index descriptors such that a chunk can query every intersecting feature, even when its owner is a neighboring region.
- Specify deterministic ownership, deduplication, tie-breaking and invalidation of cached region plans.
- Model optional world-level graphs for connections extending beyond a local region.
- Prefer strongly typed metadata; do not build a universal string-keyed giant schema.

## Required automated tests

- A feature crossing three regions has one identity but is found in all relevant spatial queries.
- Query order, thread order and cache eviction do not change plans or feature identity.
- Many features, sparse regions, overlapping bounds, partial world edges, negative coordinates.

## Explicit exclusions

No world-height voxel array, no rendering and no mandatory city/biome algorithm.

## Architecture review before coding

Approve typed WorldPlan/RegionPlan responsibility and spatial-index lifetime policy; avoid an overgeneralized inheritance tree.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
