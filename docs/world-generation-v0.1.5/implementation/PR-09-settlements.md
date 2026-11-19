# v0.1.5 — PR09: Settlements, city plans and multi-chunk structures

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core + Voxel Core. **Dependencies:** PR07–PR08.

## Objective

Describe settlements as world-scale meta-structures, with streets, plots and buildings, then realize local slices without per-chunk invention.

## Proposed scope

- Define typed settlement positions, districts, street network, lots and building footprints.
- Use suitability and exclusion checks against terrain, road connectivity and existing features.
- Ensure each building/structure has a stable ID and can span multiple chunks.
- Provide a modest deterministic demonstration planner rather than a giant mandatory city grammar.
- Paint structures through the voxel adapter while preserving authored structures/overrides.

## Required automated tests

- Towns spanning chunks, stable plot ownership, lots aligned to roads, no duplicate buildings.
- Reject unsuitable or reserved footprints deterministically.
- Generate the same town when chunks are requested in reverse order.

## Explicit exclusions

No NPC AI, economy, interiors system, procedural architectural language or gameplay population.

## Architecture review before coding

Agree on which settlement features are generic data and which belong to voxel-only recipe modules.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
