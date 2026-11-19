# v0.1.5 — PR08: Route graphs and continuous roads

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core + Voxel Core. **Dependencies:** PR04, PR06–PR07.

## Objective

Plan routes at a larger scale than chunks and project continuous road corridors across regions and terrain.

## Proposed scope

- Express paths as world-space segments/polylines with width, stable IDs and junctions.
- Offer route graphs and a small generic routing contract, without hardcoding an optimal path algorithm.
- Reserve corridors before placing scenery; expose road/structure intersection rules.
- Realize ground grading and paving into voxel chunks via existing Builder.
- Permit optional bridge, ford, ramp and tunnel descriptors without implementing full simulation.

## Required automated tests

- Seamless corridors crossing four chunks, crossing region ownership boundaries.
- Junctions, steep terrain, competing paths, crossings near world bounds.
- Same world graph yields identical local road voxels independent of chunk order.

## Explicit exclusions

No traffic simulation, NPC pathfinding or full game road-design rules.

## Architecture review before coding

Decide whether route graph planning is a reusable default algorithm or supplied by consumers.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
