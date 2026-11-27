# Voxel prefab generation — design notes (V0.1.5)

Status: design only; decisions updated on 2026-11-27. Implementation is deferred until the shadow and biome landscape showcase PRs are finalized. This work precedes city and road generation.

## 1. Scope and agreed decisions

- A voxel prefab is a reusable definition of a voxel structure intended exclusively for integration into terrain chunks: trees, bushes, rocks, fallen trunks, ruins or buildings.
- Reuse `Voxel::Volume` and existing voxel definitions for the geometry. A `Voxel::Prefab` (working name) contains a local volume and an anchor, independent of biome and world position. Treat the source definition as read-only during generation; multiple placements can reference it. Integrating a structure does not make its terrain voxels indestructible.
- There is no deployment-mode enum and no independent/flying prefab instance. A voxel chunk owns its terrain volume, not a collection of pending prefabs or separate prefab objects.
- Implement two generation passes initially: `SurfacePrefabDistributionPass` produces surface placements and `Voxel::PrefabApplicationPass` integrates them. A future `VolumePrefabDistributionPass` can distribute cave features and other volumetric structures after cave generation, reusing the same prefab definitions, probability rules and application mechanism. Volumetric distribution is explicitly deferred. They communicate exclusively through the world-level generation data (`TGenerationData` / `GenerationData`), not directly with each other.
- Sparkle provides reusable distribution orchestration and voxel application logic. The game provides prefab definitions, distribution rules and the concrete binding to its generation data. Keep the distribution/application boundary suitable for future non-voxel implementations, without introducing generic voxel storage into `spk::World`.

## 2. Biome distribution rules

Each applicable biome explicitly configures its own collection of independent prefab distribution rules (`PrefabDistributionRule`, working name). Even if two biomes require similar rules, configure them separately rather than sharing or registering rule objects. Only prefab definitions are shared. Register reusable voxel prefab definitions in `Voxel::Prefab::Registry`, following the existing voxel and biome registry conventions, and bind the registry to the generator using its existing `bind()` mechanism. Rules refer to registered prefabs with relative weights.

A natural distribution rule combines:

- A developer-supplied mathematical probability function evaluated at the absolute three-dimensional world anchor (X, Y, Z), returning a probability in [0, 1]. The surface sampling grid remains two-dimensional; compute Y from the terrain before evaluating the function.
- A collection of candidate prefab definitions with non-negative relative selection weights. Conditional on a successful placement, select among positive-weight candidates proportionally to their weights.
- A rule-specific positive grid spacing in voxels, and an optional deterministic displacement of each grid candidate to avoid artificial alignments.
- A declared finite consultation radius, allowing the rule to inspect surrounding terrain heights and biome influences in world coordinates.
- Each prefab definition specifies its allowed rotations around the vertical Y axis, chosen from 0, 90, 180 and 270 degrees. Choose an allowed orientation deterministically at placement time.

Use a world-aligned sampling grid, independent of chunk boundaries. For example, trees may be evaluated every 5 voxels, bushes every 2, rocks every 8 and flowers every voxel. The optional jitter must be bounded and derived from the stable grid coordinate, world seed and rule identity. Evaluate the probability and biome influences at the resulting displaced coordinate, not the original grid point. Account for the maximum jitter when discovering neighboring source candidates.

At the candidate location, multiply the rule probability by the influence weight of its biome: effectiveProbability = probability * biomeInfluence. For example, 70% forest influence multiplies each forest rule's probability by 0.7, while 30% prairie influence multiplies each prairie rule's probability by 0.3. Evaluate applicable rules independently; do not restrict them to the dominant biome. Perform a deterministic probability trial, then a weighted prefab selection and an allowed rotation selection. Use distinct deterministic random draws for these decisions.

The placement anchor Y coordinate is the first empty voxel immediately above the generated base terrain surface at the selected X/Z. Align the prefab's local voxel anchor to that world coordinate. The anchor alone defines its vertical offset: a prefab with an anchor above its lowest voxels is partially buried. Do not add a separate vertical offset to a natural distribution rule. Use the existing terrain height data and its consistent surface-to-voxel convention; placement must not depend on prefabs already stamped.

A forest biome may define separate rules for trees, bushes and rocks. Distinct rules can propose placements at the same coordinate, and overlaps are allowed. The distribution pass orchestrates evaluation of candidates; a natural rule does not return an arbitrary collection of placements. Organized settlements, buildings and roads will be generated by a separate future structure-generation pass using Wave Function Collapse (WFC) and adjacency constraints. Both mechanisms may reuse the prefab definition, placement representation and application mechanism, without embedding WFC in natural distribution rules.

The conceptual probability signature is `float probability(const spk::Vector3Int& position, const TGenerationData& data) const`. Reuse the concrete generation data in read-only form; do not introduce a separate Context wrapper. The function can query precomputed height maps and biome influences at neighboring world coordinates, for example to determine slope or biome transitions, as well as read game-specific generation data. The rule's declared consultation radius bounds the required neighborhood. The surface pass prepares the necessary column data before evaluating candidates, including the consultation neighborhood around displaced anchors. No neighboring chunk instantiation or cyclic generation dependencies are permitted. Neighboring data is consulted only for distribution, not collision detection; no voxel occupancy or existing prefab placement checks are required. The exact accessor names, radius units/validation and jitter bounds remain implementation details.

## 3. Placement data and determinism

A placement is a lightweight value referencing a reusable prefab definition and recording its absolute world anchor position and orientation. It has a stable identity (or equivalent stable key) derived from deterministic inputs. The data representation and exact types are to be aligned with the existing generator API.

Distribution and prefab selection must depend on seeds, absolute coordinates and stable rule identifiers, not generation order, loaded chunks, mutable shared RNG state, cache hits or thread scheduling. Natural distribution must define a stable world-grid sampling domain and candidate identity so a placement discovered by several overlapping queries is not duplicated.

The initial version has no collision detection, spatial reservations, collision masks, priority arbitration or rejection of overlapping placements. The earlier ideas of composite 3D reservation boxes and category masks are deferred, not requirements of this version.

## 4. World-level pending placements and multi-chunk integration

Pending placements belong to the world's shared generation data, not to individual chunk objects. Index pending application work by destination chunk coordinate (working representation: a world-level placement collection with a per-destination pending index). Each destination entry needs only a reference to the registered prefab definition, the world anchor position and rotation (plus any necessary stable bookkeeping identity). Several destination entries may reference the same placement. Do not copy the prefab volume or construct clipped sub-volumes; the application pass computes the intersecting region and writes directly into the destination volume.

A rule must provide a conservative finite bound for the extent of its prefabs relative to their anchors, accounting for allowed orientations and the bounded displacement of grid candidates. A bound can be derived from the definitions and allowed transforms rather than supplied manually. This lets the distribution pass discover every source region that might contribute to a requested destination, including placements anchored outside its column or in another vertical chunk.

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
- Compare the incoming prefab voxel ID with the current destination voxel ID. Replace the destination only if the incoming ID is strictly greater. Equivalently, `destinationId = std::max(destinationId, prefabId)` for non-empty source voxels. Empty voxels never erase terrain; if the empty ID is zero, the max rule already preserves this behavior.
- Apply this rule unconditionally in the initial version, including overlaps with base terrain and previously applied prefabs. Lower or equal IDs do not replace higher or equal IDs. No additional priorities, sorting or collision checks are needed. The max operation on voxel IDs is associative, commutative and idempotent, making the final IDs independent of application order. Stable identities are still useful for deduplication and pending-work bookkeeping, not for conflict precedence.
- Use world-space anchor transforms and destination-local coordinates to clip the writes. Chunk-boundary handling must work for negative coordinates and vertical neighbors as well as positive horizontal coordinates.
- No geometric collision/occupancy test is needed in this initial version. Bounding-box intersection for enumerating affected chunks and clipping writes is not collision arbitration.

Integrated orientations must preserve the voxel grid unless a future explicit resampling policy is defined. For the initial natural distribution, only the four agreed Y-axis quarter-turn orientations are supported. Rotate about the prefab's local anchor, then align it to the world anchor at the first empty voxel above the base terrain. The exact rotation API remains open. Arbitrary floating-point rotation/resampling, terrain adaptation (e.g. carving ponds or flattening building sites) and advanced merge policies are not part of the initial stamping contract.

## 6. Pass integration

The distribution pass runs once the required biome influences and terrain heights are available. The application pass runs after the base terrain/material strata have been generated, so its ID-based maximum merge takes effect before the final chunk is published.

Retain the generator's existing pass orchestration and concrete generation-data binding approach. Both passes expose access to the required data via appropriate hooks/accessors (the earlier working name was `_prefabPlacements`). Distribution publishes pending work to shared world generation data; application reads the complete target work and coordinates its successful consumption with generation finalization. A read-only per-column collection alone is no longer sufficient. Exact signatures, namespaces and ownership are to be reconciled with the implemented generator; do not impose a new generic chunk storage layout.

## 7. Future entity generation — explicitly separate

Interactive or mobile objects are entities, not independent prefabs. A future dedicated world-generation pass may determine placements for `Entity3D` instances with graphical, physical, gameplay and behavioral components. Initial streaming ownership can be based on the entity's anchor chunk, even if its graphical bounds extend outside that chunk. Entity lifetime, movement, persistence, ownership transfer and instantiation APIs are a separate design task.

A voxel volume may be reused as graphical input for an entity. Named model parts (e.g. tree trunk and leaves, rock and gold nugget) with local transforms are a possible future graphical/model composition feature, not a requirement of integrated voxel prefabs. No independent prefab conversion API or composite graphical model is required for this work.

## 8. Planned validation (not implemented)

- Multiple independent rules per biome; probability endpoints, weighted selection among multiple definitions, zero-weight candidates, allowed quarter-turn rotations, grid spacing and bounded deterministic jitter; empty regions and deterministic seed behavior.
- Biome influence weighting at transition regions; placement at the first empty voxel, with anchors at the base or inside the prefab; stable results across chunk boundaries and generation order.
- Prefabs spanning horizontal and vertical chunk boundaries, including negative coordinates and anchors outside the requested target.
- Identical results for forward, reverse and concurrent chunk generation, including overlapping prefabs; no duplicate placement discovery.
- Pending entries consumed only for the successful destination; empty pending collection is a no-op; retry, flush and regeneration reproduce the same result.
- Empty source voxels preserve the terrain; higher IDs replace lower IDs; lower/equal IDs leave the destination unchanged, including base terrain and overlapping prefabs. Verify application-order independence and repeated application idempotence for voxel IDs.
- Three-dimensional anchor passed to the probability formula; read-only generation-data access to heights and biome influences across column boundaries within the declared consultation radius; no neighboring chunk creation or collision checks.
- Bound prefab registry and explicit independent rule configuration per biome; direct clipped writes without intermediate sub-volumes.
- Visual examples of forests with several tree shapes, bushes and rocks, including deliberately overlapping foliage and structures crossing chunk boundaries. Measure generation cost and pending storage before adding collision/reservation complexity.

## 9. Out of scope / next decisions

No C++ implementation, tests, generated assets or changes to the current generator in this design commit. Collision reservations and masks, entity generation, advanced terrain shaping, WFC-based organized structure generation (including city site selection and connected roads) is later work. Volumetric prefab distribution (including caves) is deferred to a separate future pass. The remaining API questions concern concrete generation-data accessors, consultation radius and grid jitter validation, registry binding and pending-store concurrency/lifecycle implementation. No placement ordering policy remains necessary for the agreed maximum-ID merge.

Do not merge this design branch or start implementation before the agreed prerequisites are complete. The immediate next work remains the shadow pass and its visual validation.
