# PR01 — Inheritable Chunk and world-unit coordinate conversions

**Layer:** Core. **Dependencies:** none. **Status:** design draft.

## Scope
- Minimal `spk::Chunk<SizeX, SizeY, SizeZ>` inheritable base (dimensions fixed at compile time and expressed in **world units**, not voxel counts).
- Static helpers `worldToChunkCoordinate(Vector3)`, `chunkCoordinateToWorld(ChunkCoordinate)`, `worldToLocalCoordinate(Vector3,ChunkCoordinate)`.
- Signed XYZ 32-bit chunk coordinate value type/alias, using existing Sparkle math facilities wherever possible.
- **No coordinate stored in Chunk**; no Entity3D, Volume, generator logic or renderer in the base.
- Floor conversion for negatives and checked arithmetic; clear origin and inclusive boundary rules.

## Required tests
- Examples (70,-2,35) => chunk (2,-1,1), origin (64,-32,32), local (6,30,3) for dimensions (32,32,32).
- Exact positive/negative boundaries, -1, large limits, invalid dimensions, round-trip properties.
- Derived non-voxel chunk compiled and used with the same helpers.

## Exclusions
No collection, procedural terrain, region framework or rendering.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
