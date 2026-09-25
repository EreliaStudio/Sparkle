cmake_minimum_required(VERSION 3.25)

project(
    sparkle
    VERSION 0.1.3
    DESCRIPTION "C++23 engine core with optional UI, OpenGL rendering, and application runtime"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

option(SPARKLE_BUILD_GRAPHICS "Build Sparkle graphical application, input, UI and OpenGL support" ON)
option(SPARKLE_BUILD_TESTS "Build the Sparkle unit-test suites" OFF)
option(SPARKLE_BUILD_TEST_LIBRARY "Build and install reusable graphical test utilities" OFF)

set(SPARKLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SPARKLE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SPARKLE_RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

file(GLOB_RECURSE SPARKLE_CORE_SOURCES
    CONFIGURE_DEPENDS
    "${SPARKLE_SOURCE_DIR}/container/*.cpp"
    "${SPARKLE_SOURCE_DIR}/design_pattern/*.cpp"
    "${SPARKLE_SOURCE_DIR}/diagnostics/*.cpp"
    "${SPARKLE_SOURCE_DIR}/engine/*.cpp"
    "${SPARKLE_SOURCE_DIR}/math/*.cpp"
    "${SPARKLE_SOURCE_DIR}/network/*.cpp"
    "${SPARKLE_SOURCE_DIR}/structure/*.cpp"
    "${SPARKLE_SOURCE_DIR}/type/*.cpp"
)
list(APPEND SPARKLE_CORE_SOURCES
    "${SPARKLE_SOURCE_DIR}/exception.cpp"
    "${SPARKLE_SOURCE_DIR}/file_utils.cpp"
)

add_library(sparkle_core STATIC ${SPARKLE_CORE_SOURCES})
add_library(sparkle::core ALIAS sparkle_core)
set_target_properties(sparkle_core PROPERTIES
    EXPORT_NAME core
    OUTPUT_NAME sparkle_core
    CXX_EXTENSIONS OFF
    DEBUG_POSTFIX d
)
target_compile_features(sparkle_core PUBLIC cxx_std_23)

if(WIN32)
    target_link_libraries(sparkle_core PRIVATE ws2_32)
endif()

target_include_directories(sparkle_core
    PUBLIC
        $<BUILD_INTERFACE:${SPARKLE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${SPARKLE_SOURCE_DIR}
)

if(SPARKLE_BUILD_GRAPHICS)
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Stb REQUIRED)

    add_subdirectory(tools/resource_builder)
    include(cmake/EmbedResources.cmake)

    file(GLOB_RECURSE SPARKLE_GRAPHICS_SOURCES
        CONFIGURE_DEPENDS
        "${SPARKLE_SOURCE_DIR}/core/*.cpp"
        "${SPARKLE_SOURCE_DIR}/graphics/*.cpp"
        "${SPARKLE_SOURCE_DIR}/input/*.cpp"
        "${SPARKLE_SOURCE_DIR}/rendering/*.cpp"
        "${SPARKLE_SOURCE_DIR}/ui/*.cpp"
    )

    add_library(sparkle STATIC ${SPARKLE_GRAPHICS_SOURCES})
    add_library(sparkle::sparkle ALIAS sparkle)
    set_target_properties(sparkle PROPERTIES
        EXPORT_NAME sparkle
        CXX_EXTENSIONS OFF
        DEBUG_POSTFIX d
    )
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
            ${SPARKLE_SOURCE_DIR}
            ${Stb_INCLUDE_DIR}
    )
    target_link_libraries(sparkle
        PUBLIC
            sparkle_core
            GLEW::GLEW
            OpenGL::GL
    )

    if(WIN32)
        target_link_libraries(sparkle PRIVATE user32)
    endif()

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
endif()

foreach(SPARKLE_TARGET IN ITEMS sparkle_core sparkle)
    if(NOT TARGET ${SPARKLE_TARGET})
        continue()
    endif()

    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${SPARKLE_TARGET} PRIVATE
            $<$<CONFIG:RelWithDebInfo>:-gcodeview>
            $<$<CONFIG:Release>:-gcodeview>
        )
    elseif(MSVC)
        target_compile_options(${SPARKLE_TARGET} PRIVATE
            $<$<CONFIG:RelWithDebInfo>:/Z7>
            $<$<CONFIG:Release>:/Z7>
        )
    endif()
endforeach()

if(SPARKLE_BUILD_TEST_LIBRARY AND NOT SPARKLE_BUILD_GRAPHICS)
    message(FATAL_ERROR "SPARKLE_BUILD_TEST_LIBRARY requires SPARKLE_BUILD_GRAPHICS=ON")
endif()

if(SPARKLE_BUILD_GRAPHICS AND (SPARKLE_BUILD_TEST_LIBRARY OR SPARKLE_BUILD_TESTS))
    add_subdirectory(tests/TU/SparkleTestLibrary)
endif()

if(SPARKLE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

install(
    TARGETS sparkle_core
    EXPORT sparkleTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(SPARKLE_BUILD_GRAPHICS)
    install(
        TARGETS sparkle
        EXPORT sparkleTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES
        LICENSE
        THIRD_PARTY_NOTICES.md
        resources/fonts/LICENSE-Liberation.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/sparkle
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
