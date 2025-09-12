cmake_minimum_required(VERSION 3.16)
project(Sparkle VERSION 0.1.0)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/EreliaStudio/Sparkle" CACHE PATH "Install path for Sparkle" FORCE)
endif()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/add_resources.cmake)
include(CMakePackageConfigHelpers)

# Base directories
set(SPARKLE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SPARKLE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Collect all header and source files recursively
file(GLOB_RECURSE SPARKLE_HEADERS
	${SPARKLE_INCLUDE_DIR}/*.hpp
	${SPARKLE_INCLUDE_DIR}/*.h
)

file(GLOB_RECURSE SPARKLE_SOURCES ${SPARKLE_SRC_DIR}/*.cpp)


add_resources(
	"resources/textures/defaultNineSlice.png"
	"resources/textures/defaultSliderBody.png"
	"resources/font/arial.ttf"
	"resources/textures/defaultIconset.png"
	"resources/textures/defaultNineSlice_Light.png"
	"resources/textures/defaultNineSlice_Dark.png"
	"resources/textures/defaultNineSlice_Darker.png"
	"resources/textures/defaultBreak.png"
	"resources/json/mesh_renderer_shader_object.json"
)

add_compile_definitions(NOMINMAX)

# Define the library
add_library(Sparkle STATIC ${SPARKLE_SOURCES} ${SPARKLE_HEADERS})

add_dependencies(Sparkle SparkleResourcesHeaderTarget)

find_package(GLEW REQUIRED)
target_link_libraries(Sparkle PUBLIC GLEW::GLEW)

find_package(OpenGL REQUIRED)
target_link_libraries(Sparkle PUBLIC OpenGL::GL)

target_link_libraries(Sparkle PUBLIC dinput8 dxguid ws2_32)

# Include directories
target_include_directories(Sparkle
        PUBLIC
            $<BUILD_INTERFACE:${SPARKLE_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            "${CMAKE_BINARY_DIR}"
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin/$<IF:$<CONFIG:Debug>,Debug,Release>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib/$<IF:$<CONFIG:Debug>,Debug,Release>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib/$<IF:$<CONFIG:Debug>,Debug,Release>)
set(CMAKE_OBJECT_PATH_MAX 4096) # Handle deep paths

# Ensure intermediate object files go to 'obj' folder
set(CMAKE_OBJECT_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/obj/$<IF:$<CONFIG:Debug>,Debug,Release>)
	
# Architecture-based structure
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_FOLDER x64)
else()
    set(ARCH_FOLDER x86)
endif()
	
	# Update all output paths to include architecture
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ARCH_FOLDER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${ARCH_FOLDER})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${ARCH_FOLDER})
set(CMAKE_OBJECT_OUTPUT_DIRECTORY ${CMAKE_OBJECT_OUTPUT_DIRECTORY}/${ARCH_FOLDER})

# Install rules
install(TARGETS Sparkle
        EXPORT SparkleTargets
        ARCHIVE DESTINATION lib/$<IF:$<CONFIG:Debug>,Debug,Release>/${ARCH_FOLDER}
        LIBRARY DESTINATION lib/$<IF:$<CONFIG:Debug>,Debug,Release>/${ARCH_FOLDER}
        RUNTIME DESTINATION bin/$<IF:$<CONFIG:Debug>,Debug,Release>/${ARCH_FOLDER}
)

install(DIRECTORY ${SPARKLE_INCLUDE_DIR}/ DESTINATION include)

install(EXPORT SparkleTargets
        FILE SparkleTargets.cmake
        NAMESPACE Sparkle::
        DESTINATION cmake
)

set(INCLUDE_INSTALL_DIR include)

configure_package_config_file(cmake/SparkleConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake
        INSTALL_DESTINATION cmake
        PATH_VARS INCLUDE_INSTALL_DIR
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SparkleConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SparkleConfigVersion.cmake
        DESTINATION cmake
)


# Option to enable tests
option(ENABLE_TESTS "Enable building of test units" OFF)
option(ENABLE_PLAYGROUND "Enable building of playground" OFF)

if (ENABLE_TESTS)
    add_subdirectory(checker)
endif()

if (ENABLE_PLAYGROUND)
    add_subdirectory(playground)
endif()
