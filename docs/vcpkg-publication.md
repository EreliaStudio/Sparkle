# vcpkg publication plan

## Goal

Publish Sparkle in the official Microsoft vcpkg curated registry under the port name `erelia-sparkle`.

The vcpkg port name is intentionally distinct from Sparkle's CMake package and target names. The base port installs the independently consumable Core target:

```cmake
find_package(sparkle CONFIG REQUIRED COMPONENTS Core)
target_link_libraries(my_target PRIVATE sparkle::core)
```

The optional `graphics` feature installs the full graphical target:

```cmake
find_package(sparkle CONFIG REQUIRED COMPONENTS Core Graphics)
target_link_libraries(my_target PRIVATE sparkle::sparkle)
```

## Upstream preparation

- Sparkle's own source is licensed under MIT. The bundled Liberation Sans default font is separately licensed under SIL OFL 1.1, so vcpkg metadata declares `MIT AND OFL-1.1` and installs both license texts.
- The vcpkg port is named `erelia-sparkle` to avoid ambiguity with other projects named Sparkle.
- The current publication scope is x64 desktop Windows static-only (`windows & x64 & static & !uwp & !mingw & !xbox`) and is validated with the official `x64-windows-static` triplet.
- The base `erelia-sparkle` port builds Core only and has no GLEW, OpenGL or stb dependency.
- The optional `graphics` feature maps to `SPARKLE_BUILD_GRAPHICS` and adds GLEW, OpenGL and stb.
- The optional `test-library` feature maps to `SPARKLE_BUILD_TEST_LIBRARY` and requires `graphics`.
- The port must build Sparkle from source rather than consume the prebuilt Sparkle release archive.
- Sparkle's own unit tests remain disabled when building the normal vcpkg port.
- Graphical functionality is intentionally not a default feature: the curated registry requires features that add additional APIs or binaries to be opt-in.

## Current staging port

The checked-in `ports/erelia-sparkle` directory is intentionally an overlay port while this branch is under review. It builds the current checkout so CI can validate the complete vcpkg consumer path before a release tag exists. It is not the final source-acquisition block that will be copied to `microsoft/vcpkg`.

Three clean manifest-mode consumers are exercised by `tools/ci/test-vcpkg-port.ps1`:

1. base `erelia-sparkle`, proving that Core installs without Graphics or graphical dependencies;
2. `erelia-sparkle[graphics]`, proving that the full graphical CMake package is available;
3. `erelia-sparkle[test-library]`, proving that the reusable test component is available and brings in Graphics.

## Release step

The publication version prepared by this branch is `0.1.3`; existing `0.1.2` prebuilt releases remain valid and unchanged.

Before submitting the upstream vcpkg pull request:

1. Merge the publication-readiness changes into Sparkle.
2. Let Sparkle's existing `Prebuilt Windows package` workflow publish the normal `main` release for `0.1.3`. Its revision-specific tag (`sparkle-v0.1.3-g<full-commit>`) also provides the immutable GitHub source archive used by vcpkg.
3. In the port copied to `microsoft/vcpkg`, replace the checkout-relative source block with:
   ```cmake
   vcpkg_from_github(
       OUT_SOURCE_PATH SOURCE_PATH
       REPO EreliaStudio/Sparkle
       REF "sparkle-v0.1.3-g<full-commit>"
       SHA512 <archive SHA512>
       HEAD_REF main
   )
   ```
4. Obtain and pin the exact SHA512 by running the port once with `SHA512 0`, as documented by vcpkg.
5. Re-run all three external consumers using an official Windows triplet.
6. Copy `ports/erelia-sparkle` into a fork of `microsoft/vcpkg`, run vcpkg's formatting/port checks, and run `vcpkg x-add-version erelia-sparkle` so the curated registry version database is updated.
7. Submit the vcpkg change as a draft pull request first.

## Maturity note for the vcpkg pull request

Keep the maturity explanation short. The direct public predecessor is `Hyarius/Sparkle`, created on April 3, 2026. The current `EreliaStudio/Sparkle` repository is the actively maintained continuation used for publication.

Suggested wording:

> This is a new port for the current Sparkle C++ library. Sparkle has been under active public development since April 2026 in its public predecessor repository, Hyarius/Sparkle, and development later moved to EreliaStudio/Sparkle, which is the current maintained upstream. The project is intended for consumption by external projects and provides an installed CMake package with external-consumer packaging tests.

Do not include the older JGL/JGL2 lineage in the initial vcpkg pull request unless a reviewer specifically asks for additional historical context. Likewise, private SparkleV0/SparkleV1 repositories should not be used as evidence for the public-development criterion.

## Final consumer experience

A Core-only consumer should depend on `erelia-sparkle`. A graphical consumer should request `erelia-sparkle[graphics]`, and a consumer of the reusable graphical test utilities should request `erelia-sparkle[test-library]`.

Once the port is accepted into the curated registry, consumers must not need:

- a Sparkle source checkout;
- an overlay port;
- a Sparkle-specific environment variable;
- a manually downloaded Sparkle binary package;
- knowledge of Sparkle's development repository layout.
