# Voxel prefab generation — design notes (V0.1.5)

Status: design only; decisions updated on 2026-11-27. Implementation is deferred until the shadow and biome landscape showcase PRs are finalized. This work precedes city and road generation.

## 1. Scope and agreed decisions

- A voxel prefab is a reusable definition of a voxel structure intended exclusively for integration into terrain chunks: trees, bushes, rocks, fallen trunks, ruins or buildings.
- Reuse `Voxel::Volume` and existing voxel definitions for the geometry. A `Voxel::Prefab` (working name) contains a local volume and an anchor, independent of biome and world position. Treat the source definition as read-only during generation; multiple placements can reference it. Integrating a structure does not make its terrain voxels indestructible.
- There is no deployment-mode enum and no independent/flying prefab instance. A voxel chunk owns its terrain volume, not a collection of pending prefabs or separate prefab objects.
- Keep two generation passes: `PrefabDistributionPass` produces placements and `Voxel::PrefabApplicationPass` integrates them. They communicate exclusively through the world-level generation data (`TGenerationData` / `GenerationData`), not directly with each other.
- Sparkle provides reusable distribution orchestration and voxel application logic. The game provides prefab definitions, distribution rules and the concrete binding to its generation data. Keep the distribution/application boundary suitable for future non-voxel implementations, without introducing generic voxel storage into `spk::World`.

## 2. Biome distribution rules

Each applicable biome exposes a collection of independent prefab distribution rules (`PrefabDistributionRule`, working name). A rule combines:

- A developer-supplied mathematical distribution algorithm.
- A collection of candidate prefab definitions (optional relative selection weights are a proposed convenience, not a required API).
- The allowed placement orientations and bounded spatial extent of the candidate definitions.

The algorithm works on a requested region and emits zero or more placements at absolute world coordinates. It can use a regular grid, noise, deterministic random sampling, Poisson-like spacing or other strategies. A coordinate-by-coordinate optional-result algorithm remains a valid implementation of a rule, but the framework does not require scanning every voxel. Surface rules may use X/Z and read the terrain height to determine Y; the placement itself is always three-dimensional.

A forest biome may define one rule choosing among several tree prefabs, another choosing among bushes, and another for rocks or ruins. At each proposed site the rule selects a prefab from its candidates. Distinct rules can propose placements at the same position; no candidate is rejected solely because another placement overlaps it.

Rules may read the already computed biome influences and height maps, including neighboring surface data when needed. Preparing those inputs must not require constructing neighboring chunks or introduce cyclic generation dependencies. The precise policy for selecting applicable rules in biome transition regions, the read-context API and the concrete rule signatures remain open.

## 3. Placement data and determinism

A placement is a lightweight value referencing a reusable prefab definition and recording its absolute world anchor position and orientation. It has a stable identity (or equivalent stable key) derived from deterministic inputs. The data representation and exact types are to be aligned with the existing generator API.

Distribution and prefab selection must depend on seeds, absolute coordinates and stable rule identifiers, not generation order, loaded chunks, mutable shared RNG state, cache hits or thread scheduling. Region algorithms must define a stable sampling domain and candidate identity so a placement discovered by several overlapping queries is not duplicated.

The initial version has no collision detection, spatial reservations, collision masks, priority arbitration or rejection of overlapping placements. The earlier ideas of composite 3D reservation boxes and category masks are deferred, not requirements of this version.

## 4. World-level pending placements and multi-chunk integration

Pending placements belong to the world's shared generation data, not to individual chunk objects. Index pending application work by destination chunk coordinate (working representation: a world-level placement collection with a per-destination pending index). A single placement may be referenced by several destination entries; do not duplicate the prefab volume for every chunk.

A rule must provide a conservative finite bound for the extent of its prefabs relative to their anchors, accounting for allowed orientations. A bound can be derived from the definitions and allowed transforms rather than supplied manually. This lets the distribution pass discover every source region that might contribute to a requested destination, including placements anchored outside its column or in another vertical chunk.

For a target chunk:

1. Prepare the required biome/height data and evaluate the relevant source regions, including the bounded neighborhood. Register each placement for every chunk intersecting its transformed volume bounds; repeated discovery must be idempotent.
2. Before the application pass starts, guarantee that all source regions that can contribute to the target have been evaluated for this generation attempt. A target must not be finalized with missing placements from a neighboring region.
3. The application pass obtains the complete pending collection for the target from the world generation data. If empty, the pass does nothing. Otherwise it clips each transformed prefab to the target chunk bounds and writes only the voxels belonging to that chunk. No neighboring chunk need be loaded, generated or modified.
4. Once the target generation is successfully finalized, consume its pending entries from world generation data. Do not remove the entries for other destination chunks that still need the same placement. Release the shared placement record when it is no longer needed.

Example: an integrated tree anchored in chunk A also intersects chunks B and C. Generating B before A must still include B's part of the tree. Consuming B's pending work must not consume A's or C's work. The three independently generated volumes must match the result of stamping the tree once in world space.

Pending work is temporary generation state, not permanent world persistence. Consuming it must not make a later regeneration lose the structure. A regenerated target reconstructs or replays the same deterministic placements; source-region caches and completion markers must support this even after a generation-data flush. Saved player edits and persistent terrain changes are separate concerns.

### Concurrency and lifecycle contracts (implementation direction)

- Multiple requests may discover the same placement concurrently. Publication must be synchronized and deduplicated; each target receives a complete, stable snapshot of its pending placements before applying them.
- Pending work is consumed only on successful completion, not on a failed or abandoned attempt. A retry must recover the same placements, without duplicate application to a successfully completed target. Destination generation attempts must be distinguished if they can overlap.
- Bound temporary storage to regions relevant to generation and support the existing explicit generation-data cleanup mechanism. Pending work for unrequested targets can be discarded only if it can be deterministically reconstructed when required. Do not precompute or retain the entire world.
- The precise synchronization, source-region cache, generation-attempt ownership and commit/consumption API remain open. The above are correctness requirements, not a new mandatory world storage interface.

## 5. Voxel application rules

- Ignore empty voxels in the source prefab: they never erase existing terrain or previously integrated prefabs.
- A non-empty prefab voxel overwrites the destination voxel, whether it contained terrain or another prefab.
- Where multiple placements write a non-empty voxel at the same position, the last placement in a stable, deterministic application order wins. The ordering must not depend on insertion order or worker scheduling, and must be consistent across destination chunks. Use stable placement identities/keys as tie-breakers; the exact ordering key remains to be finalized.
- Use world-space anchor transforms and destination-local coordinates to clip the writes. Chunk-boundary handling must work for negative coordinates and vertical neighbors as well as positive horizontal coordinates.
- No geometric collision/occupancy test is needed in this initial version. Bounding-box intersection for enumerating affected chunks and clipping writes is not collision arbitration.

Integrated orientations must preserve the voxel grid unless a future explicit resampling policy is defined. The precise rotation API, supported orientations and anchor convention remain open. Arbitrary floating-point rotation/resampling, terrain adaptation (e.g. carving ponds or flattening building sites) and advanced merge policies are not part of the initial stamping contract.

## 6. Pass integration

The distribution pass runs once the required biome influences and terrain heights are available. The application pass runs after the base terrain/material strata have been generated, so its non-empty voxel writes take effect before the final chunk is published.

Retain the generator's existing pass orchestration and concrete generation-data binding approach. Both passes expose access to the required data via appropriate hooks/accessors (the earlier working name was `_prefabPlacements`). Distribution publishes pending work to shared world generation data; application reads the complete target work and coordinates its successful consumption with generation finalization. A read-only per-column collection alone is no longer sufficient. Exact signatures, namespaces and ownership are to be reconciled with the implemented generator; do not impose a new generic chunk storage layout.

## 7. Future entity generation — explicitly separate

Interactive or mobile objects are entities, not independent prefabs. A future dedicated world-generation pass may determine placements for `Entity3D` instances with graphical, physical, gameplay and behavioral components. Initial streaming ownership can be based on the entity's anchor chunk, even if its graphical bounds extend outside that chunk. Entity lifetime, movement, persistence, ownership transfer and instantiation APIs are a separate design task.

A voxel volume may be reused as graphical input for an entity. Named model parts (e.g. tree trunk and leaves, rock and gold nugget) with local transforms are a possible future graphical/model composition feature, not a requirement of integrated voxel prefabs. No independent prefab conversion API or composite graphical model is required for this work.

## 8. Planned validation (not implemented)

- Multiple independent rules per biome; selection among multiple definitions; empty regions and deterministic seed behavior.
- Prefabs spanning horizontal and vertical chunk boundaries, including negative coordinates and anchors outside the requested target.
- Identical results for forward, reverse and concurrent chunk generation, including overlapping prefabs; no duplicate placement discovery.
- Pending entries consumed only for the successful destination; empty pending collection is a no-op; retry, flush and regeneration reproduce the same result.
- Empty source voxels preserve the terrain; non-empty writes overwrite it; stable last-writer-wins order for overlapping placements.
- Visual examples of forests with several tree shapes, bushes and rocks, including deliberately overlapping foliage and structures crossing chunk boundaries. Measure generation cost and pending storage before adding collision/reservation complexity.

## 9. Out of scope / next decisions

No C++ implementation, tests, generated assets or changes to the current generator in this design commit. Collision reservations and masks, entity generation, advanced terrain shaping, city site selection and connected roads are later work. The remaining API questions concern the distribution-region contract, biome transition eligibility, allowed rotations, exact deterministic ordering and the pending store's concurrency/lifecycle implementation.

Do not merge this design branch or start implementation before the agreed prerequisites are complete. The immediate next work remains the shadow pass and its visual validation.
