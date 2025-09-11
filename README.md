# Installation

## Dependencies

### Visual Studio 2022 build tools

[Visual Studio 2022 LTSC download page](https://learn.microsoft.com/fr-fr/visualstudio/releases/2022/release-history#release-dates-and-build-numbers)

note: We will manually install another version of vcpkg, so you should probably opt-out of installing the vcpkg version shipped with the build tools. This can avoid some future confusion.

### LLVM

>= 18.1.5

Install either from released executable or with scoop/chocolatey.

### Ninja-build

Install from scoop/chocolatey.

### vcpkg

See [vcpkg](https://github.com/microsoft/vcpkg) to install this packet manager. Avoid using the version shipped with Visual Studio.

Set the `VCPKG_ROOT` environment variable to point to your vcpkg installation directory.

## Build and Install

Follow these steps to configure, build, and install Sparkle:

1. Clone the Sparkle repository:
```bash
git clone https://github.com/YourUsername/Sparkle.git
cd Sparkle
```

2. Configure the project with CMake:
```bash
cmake --preset [release/debug/test]
```

3. Build the project:
```bash
cmake --build --preset [release/debug/test]
```

4. Install the library:
```bash
cmake --install build/[release/debug]
```

Note: If you wish to specify a custom installation directory, you can set the CMAKE_INSTALL_PREFIX variable when configuring (back at instruction 3) like this :
```bash
cmake --preset [release/debug] -DCMAKE_INSTALL_PREFIX=<install_directory>
```
This will install Sparkle into the specified <install_directory>.

You are now ready to use Sparkle in your projects!

# Using Sparkle in Your Project

To use Sparkle in your project, you need to link against the Sparkle library and include its headers. Below are the steps:

## Example CMake Setup

Add the following lines to your `CMakeLists.txt`:

```cmake
# Find Sparkle package
find_package(Sparkle REQUIRED)

# Link Sparkle to your executable
target_link_libraries(YourProjectName PRIVATE Sparkle::Sparkle)
```
