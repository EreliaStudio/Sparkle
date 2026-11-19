# v0.1.5 proposed PR roadmap

> All PRs below are **design proposals**, not authorized implementations.
> Numbering begins at **PR01 for v0.1.5**, independent from v0.1.4's PR27.
> Old v0.1.5 planning issues #78–#83 are withdrawn; their partial
> responsibilities were reassessed and are not binding.

The plan separates a **minimal vertical slice** (PR01–PR05) from
**richer spatial features** (PR06–PR10) and **runtime integration**
(PR11–PR13). It is acceptable to reorder or split PRs after API review,
but avoid implementing streaming systems before proving generation.

| PR | Purpose | Prerequisites | Primary layer |
| --- | --- | --- | --- |
| [01](implementation/PR-01-world-coordinates.md) | Signed world/chunk/region coordinates and checked bounds | — | Generic Core |
| [02](implementation/PR-02-chunks-and-storage.md) | Small typed chunk payload and loaded collection | 01 | Generic Core |
| [03](implementation/PR-03-seeds-and-pipeline.md) | Generation context, reproducible seed derivation, stage composition | 01 | Generic Core |
| [04](implementation/PR-04-world-plans.md) | Typed world/region metadata, cross-region feature indexing | 01, 03 | Generic Core |
| [05](implementation/PR-05-voxel-first-slice.md) | Voxel::Volume adapter, basic generated ground in neighboring chunks | 02–04 | Voxel Core |
| [06](implementation/PR-06-ground-and-biomes.md) | Generic fields, ground stratification, climate/biome metadata | 03–05 | Generic + Voxel |
| [07](implementation/PR-07-scenery.md) | Deterministic scenery placement, reservations, multi-chunk objects | 04–06 | Generic + Voxel |
| [08](implementation/PR-08-paths-and-roads.md) | Route graphs, continuous corridors, ground adaptation | 04, 06–07 | Generic + Voxel |
| [09](implementation/PR-09-settlements.md) | Settlements, streets, plots, building footprints and realization | 07–08 | Generic + Voxel |
| [10](implementation/PR-10-caves.md) | 3D underground features and cross-chunk carving | 04–06, 08–09 | Generic + Voxel |
| [11](implementation/PR-11-requests-and-streaming.md) | Interest zones, request scheduling, cancellation, unloaded-chunk cache | 02, 05 | Generic Core |
| [12](implementation/PR-12-graphics-adapter.md) | Voxel meshing/render resources, adjacent chunks and invalidations | 05, 11 | Graphics |
| [13](implementation/PR-13-validation-demo.md) | Deterministic end-to-end world, stress/CI/visual integration | 06–12 | Integration |

## Review gates

**Gate A — data contracts:** approve coordinate convention, Chunk<T>
necessity, representation of region metadata, ownership and deterministic
seed policy (PR01–04).

**Gate B — first observable result:** PR05 must build and compare adjacent
Voxel::Volume snapshots headlessly from a world plan. No streaming, Graphics
or multi-threading is required for this gate.

**Gate C — coherent spatial world:** roads, scenery, settlements and optional
caves are driven by shared world-space descriptors, not invented per chunk.

**Gate D — runtime and visual world:** request/unload/rebuild safely; verify
neighbor meshing and visual goldens; demonstrate a finite world.

## Release flexibility

Ground, regions and the voxel vertical slice are foundational.
Scenery/roads/settlements/caves are planned capabilities with independent
PR scopes; the maintainer may designate some as v0.1.5 core and defer others
to v0.1.6 after architectural review. They must not be artificially
entangled just to fit a release number.

Each feature PR targets V0.1.5 and is opened separately, only after
its contracts are approved. The documentation PR is **not** PR01.
