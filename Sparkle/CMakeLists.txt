cmake_minimum_required(VERSION 3.15.2)

set(ProductName "Sparkle")

list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/glew")

list(APPEND CMAKE_PREFIX_PATH "C:/Users/JQ/scoop/apps/vcpkg/current/packages/glew_x64-windows")
find_package(GLEW REQUIRED)

add_subdirectory(src)

add_library(${ProductName} ${SOURCE_FILES})

target_include_directories(${ProductName} PUBLIC ${INCLUDE_DIRECTORIES} ${GLEW_INCLUDE_DIRS})

target_link_libraries(${ProductName} OpenGL32.lib GLEW::GLEW)