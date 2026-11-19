# v0.1.5 — PR07: Spatial placement, scenery and reservations

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core + Voxel Core. **Dependencies:** PR04–PR06.

## Objective

Place features such as trees, rocks and ruins deterministically while respecting roads, settlements and other reservations.

## Proposed scope

- Define reusable spatial placement primitives: per-region candidate generation, density, exclusion and minimum spacing.
- Use feature identities/owners independent of chunk processing order.
- Add typed footprints, bounds and exclusion/reservation queries to the regional plan.
- Realize voxel scenery via Volume::Builder only for intersecting chunks; leave future Model3D placement possible.
- Establish precedence rules for overlapping scenery and stable deduplication.

## Required automated tests

- Repeated seeds and arbitrary generation order preserve identical placements.
- Spacing and blocked footprint invariants, cross-chunk objects, biome-dependent selection.
- Overlapping regions, high-density candidates, invalid bounds, deterministic tie breaks.

## Explicit exclusions

No game-specific species list, decoration assets, animation or gameplay entity spawning.

## Architecture review before coding

Should a scenery descriptor point to voxel templates, generic recipes, or application-defined placement callbacks?

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
