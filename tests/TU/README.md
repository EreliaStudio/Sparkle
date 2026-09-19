# Sparkle unit tests

The unit-test tree is split into two targets:

- `SparkleTestLibrary` is a GoogleTest-independent static library for reusable test infrastructure. Its public headers live under `SparkleTestLibrary/includes/sparkle_test`, and tests of those helpers live under `SparkleTestLibrary/tests`.
- `SparkleTestSuite` is the GoogleTest executable. Sparkle unit-test sources live under `TestSuite/srcs`; it also compiles the helper-library tests, and all sources are discovered automatically when CMake regenerates.

Every unit test can include all reusable helpers through the umbrella header:

```cpp
#include "sparkle_test.hpp"
```

Individual headers under `sparkle_test/` remain available when a narrower include is preferred.

The suite already links `sparkle`, `Sparkle::TestLibrary`, and `GTest::gtest_main`. Test resources belong in `resources`; generated comparison results are written to the build tree.

On Windows, building `SparkleTestSuite` also builds `SparkleClipboardHolder`
beside the test executable. The clipboard contention test launches this hidden
helper process with its own native window, synchronizes acquisition/release using
events, and verifies that competing clipboard access fails. Keep the helper beside
the suite when copying test binaries. It never changes the clipboard contents;
the parent test restores its seeded text and uses bounded waits for cleanup.

Configure, build, and run either suite with its dedicated presets:

```powershell
cmake --workflow --preset testDebug
cmake --workflow --preset testRelease
```

The configure, build, and test stages are also available separately as `cmake --preset`, `cmake --build --preset`, and `ctest --preset` using the same preset name.

The test configure presets enable `SPARKLE_BUILD_TESTS` and the vcpkg `tests` feature automatically. Regular library presets do not require GoogleTest.
