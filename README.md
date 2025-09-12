## Installation

This section explains how to install the dependencies required for Sparkle and the steps to build and install the Sparkle library on your Windows computer.

### Prerequisites

To build Sparkle on Windows, the following dependencies are required:

1. vcpkg

   Sparkle uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management. vcpkg will handle the installation of all required libraries specified in the `vcpkg.json` manifest file included in the Sparkle repository.

2. CMake

   CMake version 3.16 or higher is required to configure and build Sparkle.

2. Clang-tidy

   LLVM version 18.1.5 or higher is required to configure and build Sparkle.

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

### Installing Clang-tidy

Clang-tidy is distributed as part of the LLVM toolchain. Below are two common methods for installing Clang-tidy on Windows. Make sure that your installed version is **18.1.5 or higher**.

#### Method 1: Install Clang-tidy via the Official LLVM Installer

1. Download
Download the LLVM installer for Windows from the [LLVM releases page](https://github.com/llvm/llvm-project/releases).
Choose the version **18.1.5** (or higher).

2. Installation
Run the installer and follow the on-screen instructions.

3. Verification
Verify that Clang-tidy is installed and on your PATH by opening a new terminal (Command Prompt or PowerShell) and typing:
```bash
clang-tidy --version
```
This should print the Clang-tidy version number. If you see a "not recognized" error, ensure that the LLVM bin folder (e.g., C:\Program Files\LLVM\bin) is added to your PATH environment variable.

#### Method 2: Install Clang-tidy via Chocolatey
If you have Chocolatey installed on your system, you can install LLVM (including Clang-tidy) by running:

```bash
choco install llvm
```

Once installed, verify your Clang-tidy version by running:
```bash
clang-tidy --version
```

#### Environment Configuration
If Clang-tidy is not accessible after installation, you may need to update your PATH environment variable:

1. Open Start and search for "Environment Variables".
2. Click "Edit the system environment variables".
3. In the System Properties dialog, select "Environment Variables…".
4. Under System variables, find or create a variable named PATH and append the path to your LLVM bin directory (for example, C:\Program Files\LLVM\bin).

Once Clang-tidy is installed and your environment is configured, you can confirm it is available by running:
```bash
clang-tidy --version
```

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

By default, this command installs Sparkle to `EreliaStudio/Sparkle` alongside the repository root. The resulting directory contains a `lib` folder with separate `Debug` and `Release` subdirectories.

Note: If you wish to specify a custom installation directory, you can set the CMAKE_INSTALL_PREFIX variable when configuring (back at instruction 3) like this :
```bash
cmake --preset [release/debug] -DCMAKE_INSTALL_PREFIX=<install_directory>
```
This will install Sparkle into the specified `<install_directory>`.

You are now ready to use Sparkle in your projects!

When configuring an external project, CMake needs to know where Sparkle was installed. Pass either the installation prefix or the directory containing `SparkleConfig.cmake`:

```bash
# using the install prefix
cmake -S <project> -B <build> -DCMAKE_PREFIX_PATH="C:/path/to/EreliaStudio/Sparkle"

# or pointing directly to the config directory
cmake -S <project> -B <build> -DSparkle_DIR="C:/path/to/EreliaStudio/Sparkle/cmake"
```

`find_package(Sparkle REQUIRED)` can then locate the imported target and expose the headers and libraries.

### Using Sparkle in Your Project

To use Sparkle in your project, you need to link against the Sparkle library and include its headers. Below are the steps:

#### Example CMake Setup

Here is a sample `CMakeLists.txt` that builds an executable and links it against Sparkle:

```cmake
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TAAG)

find_package(Sparkle REQUIRED)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/includes)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/srcs)

file(GLOB_RECURSE HEADERS
    ${INCLUDE_DIR}/*.hpp
    ${INCLUDE_DIR}/*.h
)
file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)

add_executable(TAAG ${SOURCES} ${HEADERS})

target_link_libraries(TAAG PRIVATE Sparkle::Sparkle)

target_include_directories(TAAG
    PRIVATE
        ${INCLUDE_DIR}
)

set_target_properties(TAAG PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/${ARCH_FOLDER}
)
```
