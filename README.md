## Installation

This section explains how to install the dependencies required for Sparkle and the steps to build and install the Sparkle library on your Windows computer.

### Prerequisites

To build Sparkle on Windows, the following dependencies are required:

1. vcpkg

   Sparkle uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management. vcpkg will handle the installation of all required libraries specified in the `vcpkg.json` manifest file included in the Sparkle repository.

2. CMake

   CMake version 3.16 or higher is required to configure and build Sparkle.

### Installing vcpkg

Follow these steps to install vcpkg:

1. Clone the vcpkg repository:
```bash
git clone https://github.com/microsoft/vcpkg.git
```

2. Navigate to the vcpkg directory and bootstrap:
```bash
cd vcpkg
.\bootstrap-vcpkg.bat
```

3. Set the `VCPKG_ROOT` environment variable:
Set the `VCPKG_ROOT` environment variable to point to your vcpkg installation directory. This allows CMake to automatically detect vcpkg.
```PowerShell
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
```
Alternatively, you can add `VCPKG_ROOT` to your system environment variables for persistent usage.

### Build and Install

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

### Using Sparkle in Your Project

To use Sparkle in your project, you need to link against the Sparkle library and include its headers. Below are the steps:

#### Example CMake Setup

Add the following lines to your `CMakeLists.txt`:

```cmake
# Find Sparkle package
find_package(Sparkle REQUIRED)

# Link Sparkle to your executable
target_link_libraries(YourProjectName PRIVATE Sparkle::Sparkle)
```
