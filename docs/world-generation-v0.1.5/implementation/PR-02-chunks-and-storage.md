# v0.1.5 — PR02: Generic chunk payload and loaded collection

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core. **Dependencies:** PR01.

## Objective

Provide a minimal way to address and cache typed chunk data, retaining revisions/invalidation information without dictating how data is generated.

## Proposed scope

- Decide whether Chunk<TData> is a meaningful value type or whether a coordinate-keyed Collection<TData> is sufficient.
- Support insert/replace/erase/find/contains and safe iteration of loaded entries.
- Define duplicate insertion, revision increments, move semantics, and stable keys.
- Store typed values only; specifically avoid duplicating Voxel::Volume cells.
- Expose a simple changed-chunk stream for consumers that need updates.
- Limit storage policy to loaded data and leave persistence/loading to later PRs.

## Required automated tests

- Insert/replacement/removal including same key, absent key and immutable payload reuse.
- Revision and changed-set ordering semantics, copy/move/lifetime, deterministic enumeration.
- Repeated updates, eviction, empty collection and non-voxel mock data payloads.

## Explicit exclusions

No asynchronous tasks, terrain generation, GPU, or generic world-sized dense grid.

## Architecture review before coding

Approve wrapper-vs-container choice and change-notification semantics; avoid an unnecessary object-per-chunk hierarchy.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
