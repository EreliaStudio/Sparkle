# v0.1.5 — PR13: Integrated sample world, reproducibility and regression suite

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Integration. **Dependencies:** PR06–PR12.

## Objective

Demonstrate that the generic planner, voxel realization, streaming and rendering together produce a coherent world under meaningful load.

## Proposed scope

- Supply one deterministic seeded finite example world with ground/biomes, scenery, routes, settlements and optional caves.
- Demonstrate generating chunks in arbitrary order, regenerating unloaded chunks and applying sparse edits.
- Demonstrate graphical loading/unloading and updates as the world is traversed.
- Capture memory and generation-time metrics and document design tradeoffs.
- Verify public package/build compatibility in CI without introducing external dependencies.

## Required automated tests

- End-to-end deterministic comparison of chunk hashes across multiple schedules and platforms.
- Reconnect/reload using existing Volume serialization as needed; no custom protocol.
- Sparkle goldens for surface, roads, buildings, cave entrance and changed-neighbor occlusion.
- Windows/Linux C++23, clang-format, sanitizer/stress tests, no skipped goldens.

## Explicit exclusions

No Erelia-specific game rules, account systems, online discovery or automatically merging the PR.

## Architecture review before coding

Final shipping scope is contingent on prior architecture reviews; identify optional late feature PRs before release.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
