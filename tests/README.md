# Sparkle unit tests

The unit tests are split along the same boundary as the installed libraries.

- `Core` builds `SparkleCoreTestSuite` and mirrors Core production folders inside `Core/Unit`, `Core/Integration`, `Core/Stress` and `Core/MemorySafety`.
  It links only `sparkle::core` and GoogleTest and is expected to run on Windows and Linux.
- `Graphics` builds `SparkleGraphicsTestSuite` and mirrors graphical production folders inside
  `Graphics/Unit`, `Graphics/Integration`, `Graphics/Golden`, `Graphics/Performance` and `Graphics/Native`. It links `sparkle::sparkle`, `Sparkle::TestLibrary`, and GoogleTest.
- On Windows, the native clipboard integration tests are extracted into
  `SparkleClipboardTestSuite`. Its custom main switches the process to a private window station
  and desktop before GoogleTest starts, so the tests use a real Win32 clipboard without reading,
  replacing, or restoring the interactive user's clipboard. Open-failure behavior is injected
  deterministically through a test-only internal clipboard hook instead of process contention.
- `SparkleTestLibrary` remains the reusable graphical test-support package.

Graphical fixtures, golden images, fonts and texture resources live under `Graphics/resources`.
The Win32 clipboard test runner lives under `Graphics/helpers`. The `TextEdit` unit tests use
an injected in-memory clipboard backend and never depend on Win32. Core tests deliberately have
no dependency on those files.

Both source trees are discovered recursively within their validation categories. Place new tests in the appropriate category and subsystem, put shared fixtures in `helpers`, and update the case-level CTest labels when special routing is required. Mixed-category fixtures may remain together to avoid duplicating setup and helpers. External consumer fixtures live in `Packaging/CoreConsumer`, `Packaging/GraphicsConsumer` and `Packaging/TestLibraryConsumer`.

See [the CI strategy](../docs/ci-test-strategy.md) for test selection and shared build artifacts.

The regular test presets build and execute both suites on Windows:

```powershell
cmake --workflow --preset testDebug
cmake --workflow --preset testRelease
```

CI compiles the standard Windows Core and Graphics suites together in Debug and Release, and builds only Core on Linux with `SPARKLE_BUILD_GRAPHICS=OFF`. Separate test jobs consume the matching executable artifacts. Independent sanitizer, coverage, compiler and packaging validations retain their own configurations.

Reusable graphical helpers remain available through:

```cpp
#include "sparkle_test.hpp"
```

Golden-image output is written beneath the Graphics build directory. CI never accepts or regenerates
reference images automatically.
