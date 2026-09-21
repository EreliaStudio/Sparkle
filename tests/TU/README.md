# Sparkle unit tests

The unit tests are split along the same boundary as the installed libraries.

- `Core` builds `SparkleCoreTestSuite` and mirrors Core production folders below `Core/srcs`.
  It links only `sparkle::core` and GoogleTest and is expected to run on Windows and Linux.
- `Graphics` builds `SparkleGraphicsTestSuite` and mirrors graphical production folders below
  `Graphics/srcs`. It links `sparkle::sparkle`, `Sparkle::TestLibrary`, and GoogleTest.
- On Windows, the native clipboard integration tests are extracted into
  `SparkleClipboardTestSuite`. Its custom main switches the process to a private window station
  and desktop before GoogleTest starts, so the tests use a real Win32 clipboard without reading,
  replacing, or restoring the interactive user's clipboard.
- `SparkleTestLibrary` remains the reusable graphical test-support package.

Graphical fixtures, golden images, fonts and texture resources live under `Graphics/resources`.
The Win32 clipboard test runner and contention helper live under `Graphics/helpers`. The
`TextEdit` unit tests use an injected in-memory clipboard backend and never depend on Win32.
Core tests deliberately have no dependency on those files.

Both source trees are discovered recursively, so a test should be placed beside the production
subsystem it validates rather than appended to a flat source list.

The regular test presets build and execute both suites on Windows:

```powershell
cmake --workflow --preset testDebug
cmake --workflow --preset testRelease
```

CI additionally configures Core with `SPARKLE_BUILD_GRAPHICS=OFF` and runs it independently in
Debug and Release on both Windows and Linux. Graphics is tested in Debug and Release on Windows.

Reusable graphical helpers remain available through:

```cpp
#include "sparkle_test.hpp"
```

Golden-image output is written beneath the Graphics build directory. CI never accepts or regenerates
reference images automatically.
