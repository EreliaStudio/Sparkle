# PR07 — Concrete voxel chunk and headless generation example

**Layer:** Voxel Core/example. **Dependencies:** PR03 and PR06. **Status:** design draft.

## Scope
- Introduce an example `VoxelChunk` deriving from `Chunk<...>` and holding existing immutable `Voxel::Volume` data. It remains legal for user chunks to hold multiple volumes, non-voxel geometry descriptions and other fields.
- Voxel size and number of cells are **independent of physical chunk dimensions**, which are world units.
- Provide a small generator using ordered user-style passes: e.g. sample heights/noise, fill terrain with Volume::Builder, add one simple deterministic cross-chunk feature.
- Demonstrate multiple configurable seeds and fresh TGenerationData sharing optional generator-owned structures.
- Handle finite-world limits using null createChunk where useful, without coupling the generic requester to voxels.

## Required tests
- Existing Volume::Builder and Definition::Registry are reused, no duplicate voxel storage.
- Neighbor continuity in XYZ including negatives, same seed/coordinate independent of request ordering, separate seeds vary output.
- Multi-chunk sample feature is coherent across boundary and works when either side generated first.
- Headless only; no volume-to-renderer direct API assumption.

## Exclusions
No mandatory city/road/cave/biome algorithms, Graphics, persistence or new serialization format.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
