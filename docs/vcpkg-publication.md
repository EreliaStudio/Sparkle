# vcpkg publication plan

## Goal

Publish Sparkle in the official Microsoft vcpkg curated registry under the port name `erelia-sparkle`.

The vcpkg port name is intentionally distinct from Sparkle's CMake package and target names:

```cmake
find_package(sparkle CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE sparkle::sparkle)
```

## Upstream preparation

- Sparkle is licensed under the MIT License.
- The vcpkg port is named `erelia-sparkle` to avoid ambiguity with other projects named Sparkle.
- The current package is Windows-only and must be validated on at least one official Windows triplet.
- The port must build Sparkle from source rather than consume the prebuilt Sparkle release archive.
- The optional `test-library` vcpkg feature maps to `SPARKLE_BUILD_TEST_LIBRARY`.
- Sparkle's own unit tests remain disabled when building the normal vcpkg port.

## Release step

Before submitting the upstream vcpkg pull request:

1. Merge the publication-readiness changes into Sparkle.
2. Publish a canonical immutable source release/tag for the selected Sparkle version.
3. Replace the local overlay source path in `ports/erelia-sparkle/portfile.cmake` with `vcpkg_from_github()`.
4. Pin the exact source revision and SHA512 required by vcpkg.
5. Validate installation and an external consumer using an official vcpkg Windows triplet.
6. Reproduce the port in a fork of `microsoft/vcpkg`, run the required port checks, and generate the vcpkg version database entry.
7. Submit the vcpkg change as a draft pull request first.

## Maturity note for the vcpkg pull request

Keep the maturity explanation short. The direct public predecessor is `Hyarius/Sparkle`, created on April 3, 2026. The current `EreliaStudio/Sparkle` repository is the actively maintained continuation used for publication.

Suggested wording:

> This is a new port for the current Sparkle C++ library. Sparkle has been under active public development since April 2026 in its public predecessor repository, Hyarius/Sparkle, and development later moved to EreliaStudio/Sparkle, which is the current maintained upstream. The project is intended for consumption by external projects and provides an installed CMake package with external-consumer packaging tests.

Do not include the older JGL/JGL2 lineage in the initial vcpkg pull request unless a reviewer specifically asks for additional historical context. Likewise, private SparkleV0/SparkleV1 repositories should not be used as evidence for the public-development criterion.

## Final consumer experience

A consumer should only need a vcpkg dependency on `erelia-sparkle` and normal CMake package discovery. It must not need:

- a Sparkle source checkout;
- an overlay port;
- a Sparkle-specific environment variable;
- a manually downloaded Sparkle binary package;
- knowledge of Sparkle's development repository layout.
