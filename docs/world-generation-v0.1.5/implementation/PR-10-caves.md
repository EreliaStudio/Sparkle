# v0.1.5 — PR10: 3D underground features and tunnel generation

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core + Voxel Core. **Dependencies:** PR04–PR06; coordinate with PR08–PR09.

## Objective

Support 3D generation rather than assuming all worlds are heightfields. Add cave/tunnel plans that span chunks and can intersect surface infrastructure.

## Proposed scope

- Introduce optional 3D density/mask and tunnel-graph feature descriptors.
- Define stable cave ownership and neighborhood queries in X/Y/Z.
- Generate voxel carving within a target chunk without modifying neighboring snapshots.
- Make cave entrances, tunnel intersections and reserved structures deterministic.
- Allow empty/solid/unknown neighboring chunks; avoid treating unloaded chunks as opaque.

## Required automated tests

- Cave crossing negative X/Y/Z chunk boundaries with no discontinuities.
- Surface entrance, cavern/tunnel intersection, under-road tunnel and foundation reservations.
- Order independence, persistence of manual overrides and cross-platform determinism.

## Explicit exclusions

No fluid simulation, cavern physics, mining gameplay or underground lighting/rendering change.

## Architecture review before coding

Approve density fields, explicit tunnel graphs, or both as generic descriptions; cave algorithms may be optional.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
