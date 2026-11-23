# Perlin noise (`spk::Perlin`)

`spk::Perlin` lives in `include/math/perlin.hpp` and is part of `sparkle::core`.
It is a mathematical utility, not a voxel-specific generator.

## API

The class retains the SparkleV1 Perlin API: `sample1D`, `sample2D`,
`sample3D`, `Interpolation::Linear`, `Interpolation::SmoothStep`,
`setSeed`, `setOctaves`, `setPersistence`, `setLacunarity`, and
`setFrequency`. Getters are available for each parameter, together with
`seed()` and `interpolation()`. A sample uses three octaves by default.

```cpp
#include "math/perlin.hpp"

spk::Perlin terrain(42u);
terrain.setFrequency(0.025f);
terrain.setOctaves(4);
terrain.setPersistence(0.5f);
terrain.setLacunarity(2.0f);

const float altitude = terrain.sample2D(worldX, worldZ, 3.0f, 48.0f);
const float cave = terrain.sample3D(worldX, worldY, worldZ, -1.0f, 1.0f);
```

Use **absolute world coordinates** when sampling across neighboring chunks.
Do not reseed per chunk: samples at the shared border must use the same
seed and same input-coordinate mapping.

## Determinism and compatibility

- Explicit seeds generate the same permutation on Linux and Windows without
  depending on the standard library's `std::shuffle` implementation.
- Seed 0 is deterministic, including the default constructor. SparkleV1
  previously made constructor seed 0 nondeterministic using `random_device`.
- Samples are independent of call order. Concurrent const sampling of the
  same instance does not mutate shared state.
- The underlying gradient lattice repeats every 256 lattice units; fractal
  sampling has a separate period per octave, controlled by frequency and
  lacunarity.
- The implementation normalizes multidimensional gradients and clamps the
  final mapped sample to the requested range. Consequently, samples are not
  guaranteed to match SparkleV1's numeric outputs for the same seed.

## Validation

`setOctaves` accepts 1 to 32. Persistence must be nonnegative and finite;
lacunarity and frequency must be positive and finite. Samples require finite
coordinates and finite, ordered bounds. Invalid parameters throw
`std::invalid_argument`. Octave coordinates exceeding the representable
float range throw `std::out_of_range`, rather than invoking undefined integer
conversions.

## Tests and performance

`tests/TU/Core/srcs/math/perlin_test.cpp` checks all three sampling
dimensions, negative coordinates, chunk-boundary continuity, periodicity,
seeded reproducibility, interpolation and octave effects, range mapping,
invalid inputs, reseeding, and concurrent const access. A separate
`PerlinPerformanceTest` prints the median nanoseconds per sample for each
dimension, without unstable wall-clock assertions.

```sh
ctest --test-dir build -R 'Core[.]Perlin' --output-on-failure
```

The Core test suite runs on Windows and Linux in Debug and Release through
the existing GitHub Actions workflow; no new CI configuration is required.

The pending PR10 terrain/golden-image fixture currently has a private
Perlin helper. After this math PR is merged into `V0.1.5`, replace that
helper with `spk::Perlin` in the PR10 branch and keep its own Graphics,
shadow, PNG and GIF tests independent.
