# PR07 — Concrete voxel chunk and headless generation example

**Layer:** Voxel Core/example. **Dependencies:** PR03 and PR06. **Status:** design draft.

## Scope
- Introduce an example `VoxelChunk` deriving from `Chunk<...>`, holding existing immutable `Voxel::Volume` data and declaring a nested **`VoxelChunk::Builder`** to construct its complete contents. Users may add other volumes and non-voxel data.
- Voxel size and number of cells are **independent of physical chunk dimensions**, which are world units.
- Provide a small generator with ordered passes on **`VoxelChunk::Builder&`**: sample heights/noise, mutate its nested `Voxel::Volume::Builder`, add one deterministic cross-chunk feature, then return the entire VoxelChunk **by value** in `VoxelChunk::Builder::build()`, consuming the builder. Also provide `VoxelChunk::Builder::load(const VoxelChunk&)`, delegating to the nested `Voxel::Volume::Builder::load(const Volume&)` as appropriate.
- Demonstrate multiple configurable seeds and fresh TGenerationData sharing optional generator-owned structures.
- Handle finite-world limits using null `createChunkBuilder` where useful, without coupling the generic requester to voxels.

## Required tests
- Existing `Voxel::Volume::Builder` and `Definition::Registry` are reused **inside the chunk Builder**; no duplicate voxel storage and no in-place edits of a published immutable Volume.
- Neighbor continuity in XYZ including negatives, same seed/coordinate independent of request ordering, separate seeds vary output; the passes never receive published immutable Volumes as mutable inputs.
- Multi-chunk sample feature is coherent across boundary and works when either side generated first.
- Headless only; no volume-to-renderer direct API assumption.

## Exclusions
No mandatory city/road/cave/biome algorithms, Graphics, persistence or new serialization format.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
