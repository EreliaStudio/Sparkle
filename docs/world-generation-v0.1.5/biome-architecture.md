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
- The generic layer should not assume Perlin, climate, a particular mesh algorithm, or a particular ground representation.

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

The numbered entries follow the design conversation. **Question 20 was not decided; it was superseded/reframed by subsequent terrain-height-generator discussions.** Therefore this document does not pretend that all 24 numbers denote approvals.

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

### 4.3 Terrain height and transitions (decisions 21–24)

21. **Biome-selected height-generator instance.** Each voxel biome references its own `spk::World::TerrainHeightGenerator` via a common abstract interface; multiple biomes may share one instance. The terrain pass invokes the selected biome's algorithm.
22. **Absolute height.** The generator returns final absolute world elevation at a position, not only a relative offset added to a biome-owned base height. Base height belongs to generator configuration.
23. **Configurable height transitions.** Hard boundaries and smoothly blended biome elevations must both be supported. **Material selection stays discrete** even when height is blended.
24. **Transitions are controlled by placement.** The biome placement system provides transition mode/distance or equivalent boundary/influence information to a terrain-height application pass. Individual height generators do not know about biome boundaries.

Height selection/blending and material selection are distinct operations. Hard transitions permit tiered cliffs; smooth transitions permit plains-to-mountains blending.

**Question 20 (superseded/not chosen):** Earlier alternatives for obtaining terrain surface height were a shared heightmap in `TGenerationData`, a callback/provider, or detection from existing geometry. The discussion then introduced a standalone, biome-selected `TerrainHeightGenerator` and did not settle where computed heights are cached or passed between generation phases. Do **not** label one of the older alternatives as approved.

## 5. Proposed pass flow (conceptual; API still open)

The intended *logical* flow is:

1. **Biome placement:** use a selectable distribution algorithm to assign each X/Z column a non-owning biome reference (or no assignment), writing to per-chunk `TGenerationData`. It may also supply the boundary/transition information needed for height blending.
2. **Terrain-height evaluation:** for each relevant position, call one or more `TerrainHeightGenerator` instances belonging to the selected/neighboring biomes; apply placement-defined hard or smooth transitions.
3. **Voxel strata application:** use each column's dominant biome and its strata to populate ground materials relative to the computed surface elevation, without mixing strata across biome boundaries.
4. **Further world passes:** add caves/voids, vegetation, prefabs, roads, towns, dungeons, scenery, etc., with coherent access to biome information.

This is a **responsibility model**, not a final mandate for exactly four C++ passes. In particular, the boundary between height evaluation and voxel filling, the stage ordering with caves/structures, and the location of shared height data are open.

The existing `spk::World::Generator<TChunk,TGenerationData,TBuilder>` already executes passes in order with `apply(builder, coordinate, data)` and creates generation data per chunk request. The current `spk::Voxel::PerlinTerrainPass` combines height evaluation and material layering; it is an implementation reference **not a constraint on this design**. No breaking changes to this API have been approved.

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

## 8. Outstanding architectural questions — not decisions

1. **Storage contract for smooth transitions:** The agreed assignment map has a single dominant biome per column, but smooth height blending also needs neighboring biome candidates, boundary distance, weights, or an equivalent query mechanism. Define this without silently reintroducing blended material strata.
2. **Height result contract:** Float/double/integer return type, world coordinate types, seed/configuration ownership, batch evaluation, and how a height value is shared or cached between passes.
3. **Application pass API:** Whether height evaluation and voxel filling are separate concrete passes; dependency on `TGenerationData`; behavior of finite strata and unassigned columns; interaction with preexisting voxels.
4. **Stratum semantics:** Thickness-to-integer rounding, unlimited-depth representation, per-stratum noise offsets, validation of weights, and how weighted noise maps to proportions.
5. **Type names and extensibility:** Exact `spk::Voxel::Biome` vs `spk::Voxel::World::Biome` name; polymorphism and lifetime-safe references; custom behaviors for biome-specific decoration and multi-chunk structures.
6. **Placement strategies:** Which reusable algorithms ship first, detailed APIs, finite-map boundaries and seed management; how environmental preferences are represented without making climate mandatory.
7. **World topology:** Surface-relative layers and 2D biome maps fit ground surfaces. Define how (or whether) a future generic 3D environment/biome placement extension coexists.
8. **Testing:** chunk seams including negative coordinates, vertical-chunk consistency, fixed-seed determinism, hard/smooth heights, distinct biome shapes, empty assignments, and future visual golden references.

## 9. Suggested next design discussion

Continue **one question at a time**. A particularly important unresolved issue is the minimal output of a biome placement pass when smooth height transitions are enabled: *how do consumers discover the neighboring height generators and their blending contributions while retaining exactly one dominant biome for stratum application?*

No implementation PR breakdown has been approved yet.

## References

- [World generation overview](README.md)
- [Existing generation contracts](contracts.md)
- [Existing world architecture](architecture.md)
- [Existing terrain generation implementation](../../include/voxel/pass/perlin_terrain_pass.hpp)
- [Existing world generator implementation](../../include/world/generator.hpp)
- [PR #99: Perlin voxel world and golden validation](https://github.com/EreliaStudio/Sparkle/pull/99)
