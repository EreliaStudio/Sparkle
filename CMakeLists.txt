cmake_minimum_required(VERSION 3.25)

project(
    sparkle
    VERSION 0.1.0
    DESCRIPTION "Sparkle static library"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Stb REQUIRED)

set(SPARKLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/srcs")
set(SPARKLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/includes")
set(SPARKLE_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

add_subdirectory(tools/resource_builder)
include(cmake/EmbedResources.cmake)

file(GLOB_RECURSE SPARKLE_SOURCES
    CONFIGURE_DEPENDS
    "${SPARKLE_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE SPARKLE_HEADERS
    CONFIGURE_DEPENDS
    "${SPARKLE_INCLUDE_DIR}/*.hpp"
    "${SPARKLE_INCLUDE_DIR}/*.h"
)

add_library(sparkle STATIC
    ${SPARKLE_SOURCES}
    ${SPARKLE_HEADERS}
)
add_library(sparkle::sparkle ALIAS sparkle)

file(GLOB_RECURSE SPARKLE_RESOURCES
	CONFIGURE_DEPENDS
	RELATIVE "${SPARKLE_RESOURCE_DIR}"
	"${SPARKLE_RESOURCE_DIR}/*"
)
list(FILTER SPARKLE_RESOURCES EXCLUDE REGEX "(^|/)\\.")

sparkle_embed_resources(
	TARGET sparkle
	ROOT "${SPARKLE_RESOURCE_DIR}"
	FILES ${SPARKLE_RESOURCES}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(sparkle PRIVATE
        $<$<CONFIG:RelWithDebInfo>:-gcodeview>
        $<$<CONFIG:Release>:-gcodeview>
    )
elseif(MSVC)
    target_compile_options(sparkle PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/Z7>
        $<$<CONFIG:Release>:/Z7>
    )
endif()

target_compile_features(sparkle PUBLIC cxx_std_23)
target_compile_definitions(sparkle PUBLIC
	UNICODE
	_UNICODE
	NOMINMAX
)
target_include_directories(sparkle
    PUBLIC
        $<BUILD_INTERFACE:${SPARKLE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${Stb_INCLUDE_DIR}
)
target_link_libraries(sparkle
    PRIVATE
        GLEW::GLEW
        OpenGL::GL
)

set_target_properties(sparkle PROPERTIES
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX d
)

install(
    TARGETS sparkle
    EXPORT sparkleTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY includes/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
    cmake/sparkleConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sparkleConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sparkle
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sparkleConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    EXPORT sparkleTargets
    FILE sparkleTargets.cmake
    NAMESPACE sparkle::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sparkle
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/sparkleConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/sparkleConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sparkle
)
