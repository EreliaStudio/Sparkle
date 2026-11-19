# v0.1.5 — PR03: Deterministic seed context and composable generation

> DRAFT DESIGN BRIEF. Do not implement until public contracts are approved.

**Base branch:** V0.1.5. **Layer:** Generic Core. **Dependencies:** PR01.

## Objective

Define reproducibility and minimal composition primitives before any ground, city, or road algorithm appears.

## Proposed scope

- Introduce a stable seed derivation contract from world seed, generator revision, stage identity, feature identity and coordinates.
- Prefer stateless hashing/explicit PRNG streams over shared mutable global randomness.
- Define a small stage/recipe composition mechanism with deterministic precedence.
- Make generation callable synchronously as the core primitive, so async scheduling can be added later.
- Keep stages domain-agnostic and allow typed input/output metadata; do not force JSON for runtime generation.
- Document floating-point reproducibility and stable ordering in collections.

## Required automated tests

- Same seed and recipe yield identical outputs on repeated calls, any chunk request order, and from multiple threads.
- Different seeds vary outputs; hashing and stage ordering have fixed reference vectors.
- Error propagation and invalid settings never publish partial results.

## Explicit exclusions

No mandatory Perlin noise, chunk streaming, game-specific assets or subclass-per-stage pattern.

## Architecture review before coding

Choose small function composition vs Builder of stages; lock algorithm/version policy for persistence compatibility.

## Delivery

Use a dedicated implementation branch and PR targeting V0.1.5; preserve existing public APIs unless the approved design requires a change. C++23, Core headless, no unnecessary indirection, Sparkle tests, clang-format, Windows/Linux CI. Never merge without explicit maintainer approval.
