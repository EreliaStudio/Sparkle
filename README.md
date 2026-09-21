# Sparkle

Sparkle is a C++23 static library for building graphical applications with a focus on UI, rendering, resources, and application runtime infrastructure.

The current public release line is validated for x64 desktop Windows and uses OpenGL. Sparkle is developed in `EreliaStudio/Sparkle`. Sparkle's own source is licensed under the MIT License; bundled third-party material retains its own license.

## Requirements

- C++23 compiler
- CMake 3.25 or newer
- Windows desktop
- OpenGL
- GLEW
- stb

The project is currently developed and validated with clang-cl using the MSVC ABI.

## CMake package

Sparkle installs a standard CMake package.

```cmake
find_package(sparkle CONFIG REQUIRED)

target_link_libraries(my_target
    PRIVATE
        sparkle::sparkle
)
```

Consumers should use the installed package rather than add Sparkle's source tree as a subdirectory.

## vcpkg

The vcpkg port name is `erelia-sparkle`. The distinct port name avoids ambiguity with other projects named Sparkle while preserving Sparkle's existing CMake package and target names.

The port is currently staged in this repository as an overlay while preparation for contribution to the official Microsoft vcpkg curated registry is completed. After upstream acceptance, consumers will be able to declare:

```json
{
  "dependencies": [
    "erelia-sparkle"
  ]
}
```

and continue using the normal CMake interface:

```cmake
find_package(sparkle CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE sparkle::sparkle)
```

See [docs/vcpkg-publication.md](docs/vcpkg-publication.md) for the publication and validation process.

## Optional TestLibrary

Sparkle can install reusable test utilities without building Sparkle's own test suite.

With the vcpkg port, enable the `test-library` feature. The installed CMake component is then consumed with:

```cmake
find_package(sparkle CONFIG REQUIRED COMPONENTS TestLibrary)
target_link_libraries(my_tests PRIVATE Sparkle::TestLibrary)
```

The TestLibrary remains independent from GoogleTest. Its OpenGL capture utilities currently require Windows/WGL.

## Prebuilt Windows packages

Sparkle also publishes revision-pinned Windows prebuilt packages for controlled downstream CI. These packages are separate from the public vcpkg distribution model and are useful for projects that want to avoid rebuilding Sparkle for every CI run.

See [docs/prebuilt-windows-package.md](docs/prebuilt-windows-package.md).

## Building Sparkle

With vcpkg available through `VCPKG_ROOT`:

```powershell
cmake -S . -B build -G Ninja `
    -DCMAKE_CXX_COMPILER=clang-cl `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build
```

To build the unit-test suite, enable `SPARKLE_BUILD_TESTS` and the root manifest's `tests` feature.

## License

Sparkle's own source is distributed under the [MIT License](LICENSE).

The bundled Liberation Sans default font is distributed under the SIL Open Font License 1.1. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [resources/fonts/LICENSE-Liberation.txt](resources/fonts/LICENSE-Liberation.txt).
