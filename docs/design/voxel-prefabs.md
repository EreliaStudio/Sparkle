# Voxel prefab generation — design notes (V0.1.5)

Status: design only. Implementation is deferred until the shadow and biome landscape showcase PRs are finalized. This work precedes city and road generation.

## Agreed decisions

- Focus on voxel prefabs (trees, bushes, rocks, fallen trunks, ruins, buildings). Keep the distribution/application boundary suitable for future non-voxel implementations without introducing generic chunk storage prematurely.
- Reuse the existing immutable `Voxel::Volume` and voxel definitions as the prefab geometry. A prefab definition contains its local volume and anchor, independent of biome, world position and deployment mode. A definition can be shared by multiple placements.
- Support two deployment modes, configured per placement/rule: fixed (`Integrated`), baked into the terrain chunk's volume; and flying (`Independent`), a separate voxel object with a local volume and its own world transform, allowing movement, rotation and individual modification. The same prefab may be used in either mode. Terrain integration does not itself imply indestructibility; editing policy is separate.
- The voxel chunk retains its terrain volume and can own independent voxel object instances. Immutable source volumes can be shared until an instance is edited. Independent objects must be rendered with their own transforms.
- Expose two generation passes, not three: `PrefabDistributionPass` decides placements; `Voxel::PrefabApplicationPass` consumes them and handles both deployment modes. Sparkle provides the reusable application logic, so game developers only supply definitions and distribution rules. Multiple specialized distributions (vegetation, scenery, ruins, etc.) can reuse the same application pass.
- Exchange placements through `World::Prefab::PlacementCollection` (proposed vector-backed container). Each placement records a prefab reference, absolute world position, orientation and deployment mode.
- Both passes expose a pure virtual accessor (working name `_prefabPlacements`) into `TGenerationData`. Distribution obtains a mutable collection for the relevant surface column; application obtains a read-only collection for the relevant chunk/column. Concrete generator subclasses bind these accessors to their own data layout. Exact signatures and namespace spelling remain to be aligned with the existing generator API.

## Determinism and cross-chunk behavior (design direction, not finalized API)

- Distribution must be deterministic from world seed and absolute coordinates, independent of generation order, loaded chunks, cache hits and thread scheduling. Use stable placement identities/tie-breaking as needed, not shared sequential RNG state.
- A fixed prefab spanning chunks must be clipped and applied to every intersected chunk. Application must be able to discover placements anchored outside the target column, based on bounded prefab extents and orientation. Define a deterministic overlap policy.
- An independent prefab is instantiated once, initially owned by its anchor chunk, without splitting its local volume. Streaming, movement across chunks, ownership transfers, culling and persistence need explicit design so objects neither duplicate nor vanish.
- Surface placement should be able to read neighboring height maps and biome influences without triggering full neighboring chunk generation or cyclic dependencies. The surface cache is an optimization, not a source of order dependence. Exact access API, bounded search, cache/threading and overlap arbitration are open questions.
- Define how empty voxels, rotations of integrated volumes, collisions, edits, physics and optional terrain adaptation (e.g. ponds) behave. Terrain-shaping passes and object placement may need different responsibilities.

## Out of scope for this design commit

No C++ implementation, tests, generated assets or changes to the current generator. City site selection and connected road generation are later work. Do not merge this design branch or start implementation before the agreed prerequisites are complete.
