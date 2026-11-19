# Sparkle v0.1.5 — World generation

> DESIGN DRAFT — no generation runtime implementation is authorized by this documentation.
> This is a replacement for the v0.1.4 implementation roadmap, not a continuation
> of its PR numbering. Approval is required before coding the feature PRs.

## Goal

Sparkle should offer a reusable, deterministic **procedural world-generation
framework**, with an optional **voxel specialization** built on the already
available Voxel::Volume and Voxel::Volume::Builder.

The framework must be able to describe the *world*, not just synthesize
independent chunks. Intended domains include terrain and ground, climate and
biomes, foliage/scenery, paths and roads, towns or cities and buildings,
underground spaces or caves, plus arbitrary application-defined features.
These are **capabilities to design**, not promises that every specialized
generator must ship in v0.1.5.

The core must work headlessly; OpenGL, EngineView and Meshers must not be
dependencies of the planning or generation layers. The engine must not encode
rules from a particular game. No new network protocol is needed.

## Documentation

1. [Architecture](architecture.md) — layering, ownership, common data and voxel adapter.
2. [Generation contracts](contracts.md) — determinism, coordinates, APIs and lifecycle.
3. [World features](features.md) — terrain, meta-structures, scenery, caves, settlements and roads.
4. [Implementation PR roadmap](roadmap.md) — proposed PRs, dependencies and scope.
5. [Validation](validation.md) — headless tests, boundary cases, performance and golden images.
6. [PR briefs](implementation/README.md) — individual proposed implementation tasks.

## Baseline

As checked on 2026-11-19, main and V0.1.5 already share the same commit:
0ed0ff4e9815ac902ec2a126b6a7ea31d67b7d63. The current Voxel::Volume is
immutable, created by Volume::Builder, and supports ByteStream/Message.
Voxel::Mesher exists in Graphics and offers an external-neighbor hook.
No generic Chunk, WorldPlanner or WorldGenerator implementation currently
exists in the public API.

The earlier GitHub issues #78–#83 are **obsolete and withdrawn**. This
document is the starting point for a newly designed plan, not their approval.

## Architecture principles

- **One source of voxel data:** only Voxel::Volume owns voxel cell snapshots.
- **Planning before rasterization:** an entire road or city can cross chunks;
  chunk generation must not independently invent a contradictory version.
- **Typed and minimal generic APIs:** domain algorithms are composable but
  do not require a giant abstract class family or a universal stringly-typed
  metadata dictionary.
- **Deterministic results:** seed, generator version, settings, coordinates
  and input metadata uniquely define a generated base chunk.
- **Explicit state layers:** base generated world, persistent/authoritative
  player edits, loaded chunks and derived rendering assets are distinct.
- **Asynchronous-friendly:** requests may run independently, but world
  publication and visible revisions happen on a controlled owner thread.
- **Progressive design:** settle class contracts and demonstrate one vertical
  slice before building many specialized world-generator implementations.

## Proposal status

Nothing in these docs defines an approved exact C++ signature or hardcoded
algorithm. Decisions still requiring review are listed in contracts.md.
Feature PRs will be created separately against V0.1.5 after agreement.
