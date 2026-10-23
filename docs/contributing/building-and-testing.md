# Building and testing contributions

Sparkle uses CMake and GoogleTest suites under `tests/TU/Core/` and `tests/TU/Graphics/`, with additional test-library and package-consumer coverage. The project supplies `CMakePresets.json`, `tools/format.ps1`, and CI workflows.

## Configure a headless build

~~~sh
cmake -S . -B build/core -G Ninja -DSPARKLE_BUILD_GRAPHICS=OFF
cmake --build build/core
~~~

To build tests, configure `SPARKLE_BUILD_TESTS=ON` with the dependencies required by the repository's test configuration, then:

~~~sh
cmake --build build/core
ctest --test-dir build/core --output-on-failure
~~~

The precise test binary, preset, dependency and platform requirements vary across Core and Graphics. For supported CI job purposes see [CI test strategy](ci-test-strategy.md) and `.github/workflows/ci.yml`. Do not claim that a change passed tests without running them on its actual commit.

## Tests as examples

The first scenario in each test group should demonstrate realistic public usage. Networking integration tests validate handshake, routing and dispatch; Engine tests show Entity/System lifecycle; graphics tests cover widgets and rendering.

See [installation](../getting-started/installation.md) for consumer builds and [writing documentation](writing-documentation.md) for keeping examples consistent.
