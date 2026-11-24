# Sparkle v0.1.5 — Biome architecture: agreed concepts and open questions

> **Status:** design record, not an implementation specification.
>
> **Branch:** `design/v0.1.5-biome-architecture` (based on `V0.1.5`).
>
> **Scope:** capture the biome-design discussion and its confirmed choices as of 2026-11-24. All API names below are conceptual unless they already exist in Sparkle. No code implementation, PR, or merge is authorized by this document.

## 1. Objective and guiding principle

Sparkle must provide reusable tools for placing, defining, and applying biomes without becoming an opinionated game-world generator. Games own their world-generation policies, but should be able to reuse biome definitions and basic passes.

A biome is an *environmental definition*, not the algorithm that distributes biome regions across the world. The distribution and application phases must be independent: different placement methods should emit the same kind of result, and downstream generation should not care which placement method produced it.

The user specifically wants:
- A first **biome placement pass** writing biome assignments into temporary generation data.
- Subsequent passes that **apply biome-driven height generation and voxel strata**, with later extension points for scenery and structures.
- Multiple interchangeable placement strategies, including manual layouts, Voronoi, environmental suitability, and competing noise fields.
- A minimal generic `spk::World` layer, with voxel specialization and game-specific extensions.

Do not infer a fully finalized C++ API from these principles.

## 2. Three concrete target games

These cases are architectural tests, not engine-predefined biome types.

### Game A — Finite, tiered/plateau world

- Finite world generated once, deterministically from a seed; terrain is not intended to be editable.
- Several regions at discrete levels, commonly 3–4 voxels high, with cliffs and staircases at changes in elevation.
- A predefined number of chunk-sized/voxel regions, each with a biome, base elevation, ground strata, and eventually scenery, buildings, and towns.
- Chunk-scale ("chunky") biome assignment must be possible.

### Game B — Large floating island

- A large volume of terrain floating in void, containing dungeons, forests, swamps, mountains, etc.
- Each island has a main biome with a predefined collection of sub-biome variations.
- Placement and local variations should preserve the main biome's identity while allowing variety.
- A conventional heightmap does not fully represent floating-island undersides, caves, or overhangs; separate 3D/volumetric generation must remain possible.

### Game C — Minecraft-like world

- Streaming/deterministic voxel terrain; biome shape and ground materials must differ significantly.
- Examples include relatively flat deserts, canyon/mesa plateaus, and narrow, steep Chinese-style mountain pillars.
- Some terrain borders should be hard (cliffs), others smooth.

**Cross-case requirement:** terrain-shape algorithms, biome placement, biome content, and eventual decoration/structure generation must not be fused into a single monolithic algorithm.

## 3. Conceptual architecture

### 3.1 Generic world layer

- `spk::World::Biome`: an **empty generic base**, with no mandatory identifier or environmental fields. A polymorphic destructor may be needed, but its API has not been finalized.
- `spk::World::TerrainHeightGenerator`: a separate **generic abstract height-generation algorithm**. It is **not** under `spk::Voxel`, and must not depend on voxels or on biome types.
- A height generator evaluates the **absolute world elevation** for an X/Z position. Base altitude is a property of the generator's configuration, **not** a value that the biome is required to add.
- The generic placement concept associates spatial positions with biome definitions; no mandatory registry or numeric biome ID.
- The generic biome/placement layer should not require climate, a particular mesh algorithm, or a particular ground representation. A separately defined, concrete `spk::World::HeightMap` now offers a Perlin-based generic height source and transient sampled grids (see §4.4). How the previously proposed abstract `TerrainHeightGenerator` and biome-selected sources integrate with it is still to be reconciled; neither concept should be silently conflated.

### 3.2 Voxel layer

- A proposed `spk::Voxel::Biome` (exact namespace/name remains open; `spk::Voxel::World::Biome` was also mentioned) inherits the generic biome and holds:
  1. A **non-owning reference** to a generic terrain-height generator.
  2. An **ordered collection of voxel ground strata**.
- Strata define ground composition; height generators define terrain elevation and shape. A biome therefore influences both the shape and the material appearance of generated terrain.
- Voxel generation consumes results but must not be tied to greedy meshing; other voxel meshing methods should be able to use the same biome descriptions, subject to representation-specific adapters.

### 3.3 Game-specific layer

- Games can derive biome types with additional properties or executable behavior for terrain/scenery/towns/dungeons.
- The desired future architecture includes hooks/extensions for game-specific biome operations. **The hook signature, invocation frequency, ordering, lifetime, and interaction with multi-chunk structures remain undefined.**
- It is not decided whether scenery/structure behavior lives in biome virtual hooks, independent passes, or both.

## 4. Confirmed decisions

The numbered entries follow the design conversation. **Question 20 was explicitly revisited and validated after decisions 21–24.** Decisions 1–24 remain recorded. Subsequent HeightMap-specific decisions, with more concrete API contracts, appear in §4.4.

### 4.1 Voxel strata (decisions 1–12)

1. **Surface-relative strata.** Ground composition is measured downward from the terrain surface, not from fixed world Y intervals.
2. **Multiple voxel types per stratum.** Each stratum holds a weighted collection of candidate voxel IDs.
3. **Configurable material distribution per stratum.** Initially envisioned as independent weighted selection, 2D noise-based distribution, or an extensible custom strategy.
4. **Variable stratum thickness.** A stratum has minimum and maximum thickness/depth; normalized 2D Perlin noise selects thickness spatially. Fixed thickness is the degenerate min=max case.
5. **Independent Perlin configuration.** Each stratum's thickness variation has its own Perlin settings (e.g. seed/frequency), instead of forcing one shared noise field.
6. **Discrete material composition at biome borders.** Each terrain column uses only the *dominant* biome's strata; mixing different biomes' stratum definitions is not required.
7. **Numeric voxel IDs.** Weighted materials are referenced by voxel ID; no mandatory material registry or object-reference resolution.
8. **2D material distribution.** Selection uses world X/Z; within a given stratum, the selected material stays consistent along that column's Y extent. Material-distribution noise and thickness noise are separate.
9. **Optional unbounded final stratum.** A biome may have a final infinite stratum or terminate at finite depth. Beyond a finite final stratum, the biome specifies no material; that does *not* imply air.
10. **Solid materials only.** Empty/air voxel ID 0 is not a candidate stratum material. Caves and voids are handled by other generation processes.
11. **Relative material weights.** Weights need not sum to 100%; they are normalized for selection. At least one usable positive weight is needed. Exact weight validation and distribution mapping remain to be specified.
12. **No special surface layer type.** The first stratum is the surface; snow and similar overlays may be addressed separately by later passes.

Example (illustrative only):

| Stratum | Thickness | Candidate materials (relative weights) | Distribution |
| --- | --- | --- | --- |
| Surface | 1 | Grass: 9, Dirt: 1 | 2D weighted/random or noise |
| Subsurface | 2–6 | Dirt: 8, Gravel: 2 | 2D noise |
| Deep | Unbounded | Stone: 9, Granite: 1 | 2D noise |

A stratum's thickness Perlin configuration is independent from its material-selection distribution.

### 4.2 Biome identity, references, and temporary placement (decisions 13–19)

13. **Empty generic biome base.** `spk::World::Biome` has no mandatory data members. Biome identifiers, if ever used, would be external.
14. **Definitions are shared.** One biome definition can be reused in many regions. A mandatory ID-based collection was considered, then explicitly rejected in favor of the original shared-definition option.
15. **Direct references rather than IDs.** Placement data refers to shared biome objects directly, with no ID-resolution requirement.
16. **External ownership.** The developer/world setup owns biome instances. Placement results store **non-owning references**; referenced objects must outlive all relevant generation work.
17. **Temporary per-chunk storage.** The placement pass writes its results into the existing `TGenerationData` of `spk::World::Generator`. An external, persistent Voronoi/world map may still be the placement algorithm's *input*, without requiring persistent biome output storage.
18. **2D biome assignment per voxel column.** The temporary output stores a biome reference per chunk-local X/Z column. A `16×16×16` voxel chunk would contain 256 entries. Vertical chunks at the same world X/Z must resolve the same biome, using absolute coordinates.
19. **Unassigned columns are allowed.** A missing biome reference (conceptually nullable) means biome application skips that column. No mandatory fallback biome.

**Important nuance:** The game may request a "biome per chunk" visual/style mode by assigning the *same biome* to all columns within a chunk-sized area. The canonical output can still be the per-column 2D map. Whether a separate coarser representation is useful is unconfirmed.

### 4.3 Terrain height and transitions (decisions 20–24)

20. **Dedicated terrain-height pass.** After biome placement, a separate terrain-height pass evaluates the selected biome's `spk::World::TerrainHeightGenerator` (and any neighboring influences needed for placement-defined transitions), and writes an **absolute surface-height map into the current chunk's `TGenerationData`**. The subsequent voxel biome application pass reads that stored map to apply surface-relative strata. Computing height inside the voxel application pass is not the selected design. The height-map storage representation has since been refined to a transient `HeightMap::Grid` held through a pooled `Lease` (see §4.4); behavior for unassigned biome columns remains open.
21. **Biome-selected height-generator instance.** Each voxel biome references its own `spk::World::TerrainHeightGenerator` via a common abstract interface; multiple biomes may share one instance. The terrain pass invokes the selected biome's algorithm.
22. **Absolute, continuous height.** The generic height-generation algorithm produces an absolute **continuous** world elevation, not an integer voxel altitude or only a relative offset added to a biome-owned base height. Base height belongs to generator configuration. Voxel consumers are responsible for conversion to discrete voxel coordinates; the conversion and rounding rule remain undecided.
23. **Configurable height transitions.** Hard boundaries and smoothly blended biome elevations must both be supported. **Material selection stays discrete** even when height is blended.
24. **Transitions are controlled by placement.** The biome placement system provides transition mode/distance or equivalent boundary/influence information to a terrain-height application pass. Individual height generators do not know about biome boundaries.

Height selection/blending and material selection are distinct operations. Hard transitions permit tiered cliffs; smooth transitions permit plains-to-mountains blending.

### 4.4 HeightMap, Grid and generic generation pass — confirmed design (2026-11-24)

The later discussion supersedes older, exploratory alternatives elsewhere in this document. This section records **confirmed decisions**, not yet compiled or implemented code. Precise header/type names remain to be checked against existing Sparkle conventions.

#### 4.4.1 Separation of responsibilities

- `spk::World::HeightMap` is a **concrete, non-polymorphic procedural height source**, not an abstract base to derive for world-specific transformations.
- `HeightMap` owns a base altitude (default `0.0f`) and a **named associative collection** of independent Perlin noise layers. The collection is a `std::map<std::string, NoiseLayer>` (a collection, **not** an index-based vector/container); expose mutable and const references for direct insertion, lookup, modification and removal. No `addNoiseLayer()` API is necessary.
- Each `NoiseLayer` **owns a copy** of its `spk::Perlin`, together with `minimum` and `maximum` contribution bounds. Existing `spk::Perlin::sample2D(x, z, minimum, maximum)` supplies the remapping directly; do **not** modify `spk::Perlin` to give it persistent min/max values. An illustrative layer is `{perlin, -30.0f, 80.0f}`.
- Height at an absolute world X/Z position is `baseHeight + sum(layer.perlin.sample2D(worldX, worldZ, layer.minimum, layer.maximum))`. An empty collection yields flat ground at `baseHeight`. Height values stay continuous `float`; voxel-specific integer conversion belongs to voxel consumers.
- `HeightMap` is concerned only with **initial** height generation. Subsequent user-defined generator passes edit the resulting per-chunk `Grid`, e.g. lowering elevations toward the coast of a finite island. They do **not** mutate the procedural definition or require inheriting `HeightMap`.
- Configuration/layer edits are permitted **between** generation jobs, not concurrently with sampling from the same `HeightMap`.

#### 4.4.2 Grid::Configuration, world extent and resolution

`HeightMap::Grid::Configuration` exposes **only** the grid's two-dimensional world-space `size` and sampling `scale`. `size` means **world footprint/extent**, **not** the number of stored samples. `scale` means **world units between adjacent samples**, and must be strictly positive and finite. Configuration may be written illustratively as:

```cpp
struct Configuration
{
    spk::Vector2UInt size; // World-space extent in X/Z, not sample counts
    float scale = 1.0f;    // World-space spacing between samples
};
```

- **Both endpoints are always included.** The earlier `EndBoundaryExclusion` enum and its alternate boundary policies were explicitly **removed**. There is no optional outer-border mode.
- **Required invariant on each axis:** `size.x / scale` and `size.y / scale` must both be nonnegative **integers** (within a carefully specified floating-point comparison tolerance). Reject any configuration for which either quotient is fractional; **never** silently round it or introduce an uneven final spacing.
- **Sample counts:** `sampleCountX = size.x / scale + 1`, `sampleCountY = size.y / scale + 1`. This `+1` is **one full sample interval of `scale` in world coordinates**, not an assumed unit-distance sample. Its position coincides with the supplied world-space extent, not a position beyond it.
- **Valid local positions:** `[0, size.x]` and `[0, size.y]`, including both endpoints. Every `at()` overload rejects a position outside this rectangle; e.g. size=16, scale=1: 16 is valid; 16.1 throws. There is **no extended query region** beyond `size`.
- Examples per axis:
  - `size=16, scale=1` → `17` samples at `0,1,...,16`.
  - `size=16, scale=2` → `9` samples at `0,2,...,16`.
  - `size=16, scale=4` → `5` samples at `0,4,8,12,16`.
  - `size=16, scale=0.5` → `33` samples at `0,0.5,...,16`.
  - `size=16, scale=3` → **`spk::Exception`**, since `16/3` is not an integer.
  - `scale=0`, a negative or non-finite scale, invalid size, or overflow of sample counts/capacity → **`spk::Exception`**.
- Validation is performed at the beginning of **`HeightMap::generate()`**, **before** obtaining/allocating a pooled grid. Constructing a `Configuration` alone is not expected to validate it.

#### 4.4.3 Grid local-coordinate access and interpolation

All public `Grid::at()` coordinate arguments are **`spk::Vector2` local world-distance coordinates**, never `Vector2UInt` sample indices. No implicit conversion to world coordinates is exposed by these methods. Suggested API:

```cpp
float& at(const spk::Vector2& localPosition);
const float& at(const spk::Vector2& localPosition) const;

enum class Interpolation { Nearest, Bilinear };

float at(
    const spk::Vector2& localPosition,
    Interpolation interpolation) const;
```

- **No enum:** validate the local position lies in `[0,size]` on both axes; select sample index by `floor(localPosition / scale)` componentwise; return a **mutable or const reference** to the stored height. For instance with scale=1, `at({15.5, z})` aliases the sample at X=15, while `at({16, z})` aliases X=16. Exception on an out-of-range position.
- **`Nearest` enum:** validate the same local world-space domain; return a **`float` by value** corresponding to the mathematically nearest stored sample. Exactly half-way ties choose the **lower** sample index. If the operation somehow requires an unavailable sample, throw. With the now-always-included final sample and the valid `[0,size]` domain, normal in-range nearest queries have the required samples.
- **`Bilinear` enum:** validate the same domain; interpolate using existing adjacent stored samples and return a **`float` by value**. At an exactly aligned sample, including the final edge/corner, that sample's height is a valid result; **do not** require a nonexistent sample beyond the edge with zero interpolation weight.
- Every overload uses `spk::Exception` for invalid positions or missing required samples; no automatic clamping or extrapolation. Non-finite positions are invalid.
- Using a shared `Vector2` interface avoids exposing integer sample indices, while the no-enum overload remains suitable for mutating per-column/per-sample heights.

#### 4.4.4 Generation, ownership and pool tiers

The intended public generation API is:

```cpp
[[nodiscard]] Grid::Lease generate(
    const Grid::Configuration& configuration,
    const spk::Vector2& worldOrigin) const;
```

- `HeightMap` receives the requested configuration and explicit **world origin**, validates it, computes sample dimensions, chooses a capacity tier, acquires/reconfigures a `Grid`, fills all height samples, and returns its RAII `Lease`.
- **The caller/pass never computes sample counts, selects a pool or resizes storage.** In particular, the generic generation pass only supplies configuration and origin, then stores the resulting lease.
- Pools are **static and shared by all `HeightMap` instances**, grouped by **power-of-two capacity measured in total number of stored samples**. E.g. `17×17 = 289` samples use the `512`-sample tier, `9×9 = 81` use the `128`-sample tier. Each tier's `spk::Pool<Grid>` reuses its grids and retained underlying allocations; a grid's logical dimensions, world origin and sample values are reset/overwritten each time.
- A registry/collection of pools needs synchronization for tier creation/lookups; individual `spk::Pool` instances already implement thread-safe obtains/recycles. Protect lifetime and avoid mutable configuration during parallel generation.
- `Grid::Lease` denotes a `spk::Pool<Grid>::Lease` (exact alias location/name to settle). Existing `Pool<T>::Lease` is default-constructible and move-assignable: it can be held directly in `TGenerationData` and replaced without an `optional`.
- **No eviction, maximum idle count, or pool trimming** in v0.1.5. Pools retain freed grids; reconsider memory caps only if measurements justify it.

#### 4.4.5 Generic HeightMapGenerationPass (two hooks)

- A reusable **abstract `HeightMapGenerationPass<TChunk,TGenerationData,TBuilder>`** derives from the existing `spk::World::Generator<...>::Pass` and fully implements its `apply(builder, chunkCoordinate, data)`. Game/voxel-specific subclasses only supply information through **two pure virtual hooks**:
  1. `_gridConfiguration(chunkCoordinate, const TGenerationData&)` → `Grid::Configuration`.
  2. `_gridDestination(TGenerationData&)` → a **reference to the `Grid::Lease` storage** to replace in `TGenerationData`.
- The pass has a non-owning `bind(HeightMap&)` connection to the concrete height source. Its lifetime must cover pass execution, and rebinding while jobs are running is unsupported. A separate virtual hook for selecting the `HeightMap` was **not** chosen.
- The pass calculates the world-space origin from **chunk coordinates × configured world-space `size`** (componentwise). It does **not** need an origin hook, nor does it use a sample count for the origin. The resulting origin and configuration are passed to `HeightMap::generate()`.
- `apply()` then move-assigns the returned `Lease` into the referenced destination. The intermediate grid is subsequently accessible to custom passes that alter the terrain, followed by voxel/mesh application passes. No changes to the existing `Generator` template or a concept constraining `TGenerationData` are needed.
- Specific signatures for coordinates and error handling when unbound should be finalized against actual Sparkle headers before implementation.

## 5. Proposed pass flow (heightmap API confirmed; biome integration still open)

1. **Biome placement:** assign a dominant biome or no biome for each X/Z column in `TGenerationData`, optionally with terrain-height transition metadata.
2. **Generic heightmap generation:** the bound `HeightMapGenerationPass` gets the grid world-space `Configuration` and lease destination through two subclass hooks. It derives the world origin from chunk coordinate × world-space grid size and calls `HeightMap::generate(configuration, origin)`, which handles all sampling and static pool selection. It stores the resulting `Grid::Lease` in `TGenerationData`.
3. **Optional custom grid-modification passes:** edit heights in the transient grid (e.g. lower the edge of a finite island, level a road/town) without subclassing or mutating the procedural `HeightMap`.
4. **Voxel strata/terrain application:** use final sampled heights and dominant biome strata to populate voxel columns; other consumers may use height interpolation for non-voxel surfaces.
5. **Future world passes:** vegetation, prefabs, caves, roads, buildings etc., with coherent biome input.

**Still unresolved:** Earlier decisions envisaged each voxel biome referring to a generic `TerrainHeightGenerator` and having placement-controlled hard/smooth height transitions. The newly approved simple concrete `HeightMap` (with its own named Perlin layers) is **not yet reconciled** with biome-specific algorithms and transition blending. Do not quietly equate the two or claim multi-biome blending has been implemented/designed.

The existing `spk::World::Generator<TChunk,TGenerationData,TBuilder>` already calls passes in order with `apply(builder, coordinate, data)`. The current `spk::Voxel::PerlinTerrainPass` remains an implementation reference, not a constraint. No breaking change to `Generator` has been approved.

## 6. Candidate reusable biome-placement algorithms (not yet approved as final APIs)

All candidates should have an equivalent observable output: per-column biome assignments and, where applicable, transition metadata. Their internal computation may differ.

- **Manual/explicit regions:** regions or tiles predetermined by the game; useful for the finite tiered world.
- **Finite Voronoi regions:** sites distributed over a bounded map; Poisson-disk sampling is a candidate for well-spaced site positions; assign selected biome definitions to sites/cells. A **biome per chunk** behavior and a **biome per voxel column** behavior should both be expressible (potentially as different sampling granularities into the same per-column output).
  - **Coverage caveat:** Poisson-disk sampling only spaces points. It does **not** by itself guarantee every biome type appears. If coverage is required, biome-to-site assignment must guarantee at least one site per requested biome (and there must be enough sites).
- **Environmental suitability:** evaluate world variables such as temperature, moisture, elevation, or other game-defined conditions; choose the biome most favored by each location. Whether preferences belong in base biome data, game-derived biome data, or placement rules remains open.
- **Competing noise fields:** one Perlin/noise field per candidate biome, with the highest-scoring field selecting the region.
- **Cellular automata:** a previously suggested option for iterative, neighborhood-driven distributions; requires special thought for determinism and chunk independence.
- **Hierarchical/main and sub-biome placement:** needed for the island game, with a constrained set of variations per parent environment; no mandatory inheritance or fixed hierarchy has been approved.

A placement algorithm may consult a persistent finite map or deterministic world-scale state, but should provide temporary per-chunk results to downstream passes. Chunk-order independence, seams, and negative coordinates must be tested.

## 7. Scope boundaries and compatibility

- The core biome concept is not inherently voxel-specific.
- A heightmap is a reusable terrain-surface model, **not** a universal model for caves, overhangs, floating islands, or arbitrary 3D shapes. Volumetric terrain should remain a separate, compatible extension.
- Voxel strata describe **material composition**; they do not decide air/solid topology, caves, or void geometry.
- Voxel biomes should remain independent of rendering and meshing choices. Marching cubes or alternative volume encodings may need an adapter; representation compatibility is not yet demonstrated.
- `TGenerationData` is temporary per generation request. Non-owning biome and height-generator references require valid external lifetime and read-safe configuration during concurrent chunk generation.
- The design should preserve deterministic generation across independent chunks and avoid recomputing the same biome decision in unrelated downstream passes.
- Mutable-after-setup behavior and cache invalidation are not designed yet.

### Approved initial ground-generation scope (2026-11-24)

The first implementation phase will focus on **heightmaps, biome placement/selection, biome-driven terrain-height generators, and voxel strata**. This phase is meant to develop reusable world-generation infrastructure while progressively enriching the existing Sparkle chunk-rendering/golden-image validation world. The validation scene is an evolving example of a fuller world, **not** a special-purpose permanent substitute for production APIs.

Only **after** these ground-generation building blocks are defined, implemented, and validated will the discussion move to **prefab placement passes** (trees, rocks, scenery, and subsequently buildings or other structures). Arbitrary 3D/volumetric terrain, such as cave geometry, overhangs, and floating-island undersides, is intentionally deferred; the generic interfaces should not preclude such future extensions.

This scope decision does not settle the previously proposed alternatives concerning whether to replace the existing Perlin fixture immediately or retain it as a parallel regression baseline. Golden-image and chunk-streaming regression coverage should be preserved.

**HeightMap representation selected:** `spk::World::HeightMap` is a concrete procedural height generator, with transient, mutable and pooled `HeightMap::Grid` sample storage and both reference-returning direct `at(position)` and value-returning interpolated `at(position, mode)` queries. Subsequent passes modify the sampled grid rather than the procedural generator. See the detailed, later-approved contract in §4.4.

## 8. Outstanding architectural questions — not decisions

1. **Storage contract for smooth transitions:** The agreed assignment map has a single dominant biome per column, but smooth height blending also needs neighboring biome candidates, boundary distance, weights, or an equivalent query mechanism. Define this without silently reintroducing blended material strata.
2. **Remaining HeightMap integration details:** The primary procedural/grid contract is **confirmed in §4.4**. Still to settle: exact C++ type/alias names; representation of `Grid`'s storage; numeric edge cases for floating-point divisibility tolerance; reference semantics for `at()` at boundaries; handling unassigned biome columns; voxel integer rounding; and how biome-selected height algorithms and blending integrate with the current `HeightMap` design.
3. **Application pass API:** Concrete interfaces and `TGenerationData` access for the already-separated height and voxel application passes; behavior of finite strata and unassigned columns; interaction with preexisting voxels.
4. **Stratum semantics:** Thickness-to-integer rounding, unlimited-depth representation, per-stratum noise offsets, validation of weights, and how weighted noise maps to proportions.
5. **Type names and extensibility:** Exact `spk::Voxel::Biome` vs `spk::Voxel::World::Biome` name; polymorphism and lifetime-safe references; custom behaviors for biome-specific decoration and multi-chunk structures.
6. **Placement strategies:** Which reusable algorithms ship first, detailed APIs, finite-map boundaries and seed management; how environmental preferences are represented without making climate mandatory.
7. **World topology:** Surface-relative layers and 2D biome maps fit ground surfaces. Define how (or whether) a future generic 3D environment/biome placement extension coexists.
8. **Testing:** chunk seams including negative coordinates, vertical-chunk consistency, fixed-seed determinism, hard/smooth heights, distinct biome shapes, empty assignments, and future visual golden references.

## 9. Suggested next design discussion

Continue **one question at a time**. The `HeightMap::Grid` size/scale/boundary/access contract and static tiered pool design are established in §4.4. The next architectural challenge is reconciling the concrete, Perlin-layered `HeightMap` with biome-selected height generation and smooth/hard biome transitions: *how do consumers discover neighboring height sources and blending contributions while retaining exactly one dominant biome for stratum application?*

No implementation PR breakdown has been approved yet.

## References

- [World generation overview](README.md)
- [Existing generation contracts](contracts.md)
- [Existing world architecture](architecture.md)
- [Existing terrain generation implementation](../../include/voxel/pass/perlin_terrain_pass.hpp)
- [Existing world generator implementation](../../include/world/generator.hpp)
- [PR #99: Perlin voxel world and golden validation](https://github.com/EreliaStudio/Sparkle/pull/99)
