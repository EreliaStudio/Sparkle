# v0.1.5 — PR01: World coordinates and spatial bounds

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core. **Dependencies:** None.

## Objective

Agree on a single integer spatial convention for world, chunk, and region addressing. Provide checked conversions and explicit finite or unbounded world bounds without creating a new math library.

## Proposed scope

- Use or adapt existing Sparkle vector types where they satisfy the signed-coordinate contract.
- Specify floor division/modulo for negative coordinates (e.g. world -1 goes to previous chunk, local size-1).
- Support 3D chunks and 2D regional planning, with configurable strictly positive dimensions.
- Clearly specify world origin, unit scaling, overflow errors and boundary inclusivity.
- Keep region addressing separate from voxel payloads and graphics coordinates.

## Required automated tests

- Tables of positive/negative locations, including -1, exact negative multiples, 0 and max bounds.
- Round-trip world-to-chunk-to-world tests and random/boundary property tests.
- Invalid dimensions, very large coordinates, overflow and finite-world edge cases.

## Explicit exclusions

No chunk payload, generation policy, noise, networking or renderer.

## Architecture review before coding

Choose named coordinate types vs existing TVector wrappers; formalize 2D/3D region policy before implementing.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
