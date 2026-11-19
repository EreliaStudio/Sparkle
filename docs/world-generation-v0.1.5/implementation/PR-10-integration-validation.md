# PR10 — End-to-end sample and release validation

**Layer:** Integration + Graphics tests. **Dependencies:** PR07–PR09. **Status:** design draft.

## Scope
- Complete example: generic Chunk base, `TChunk::Builder` with voxel Volume builder, pass-based Generator finalizing the chunk only after all passes, collection with Entity3D, local Task requester, WorldObserver-driven streaming and event-driven Graphics adapter.
- Show loading/unloading while moving a player, replacement of chunk data while keeping entity/component identity, and a headless equivalent with two observers.
- Exercise an intentionally limited world (null outside bounds), plus signed-coordinate large sparse-space checks.
- Update high-quality golden images for visible voxel surfaces, including cross-chunk transitions and renderer updates.
- Ensure docs reflect the actual public API rather than draft spellings.

## Required tests
- Run Core headless and Graphics integration suites on Windows/Linux CI with C++23 and clang-format.
- Regression for coordinate floor, builder pass order and failed-finalization safety, cache-free unload, timeout supersession, late subscriber scan, chunk replacement, entity teardown and cross-chunk geometry.
- No skipped goldens. Use Sparkle's built-in comparator, not Python.
- Verify no graphics dependency in generic world-generation library and no unapproved protocol changes.

## Exclusions
No large default voxel terrain catalog, road/city/cave engine, app/game-specific rules, AI or gameplay networking.

## Release gate
All relevant CI passing and explicit maintainer approval. No merge without approval.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
