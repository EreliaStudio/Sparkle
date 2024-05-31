cmake_minimum_required(VERSION 3.15.2)

set(ProductName "Sparkle")

find_package(GLEW REQUIRED)

add_subdirectory(src)

add_library(${ProductName} ${SOURCE_FILES})

target_include_directories(${ProductName} PUBLIC
    $<BUILD_INTERFACE:${GLEW_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:include/${ProductName}>
)

target_include_directories(${ProductName} PUBLIC
    $<BUILD_INTERFACE:${INCLUDE_DIRECTORIES}>
    $<INSTALL_INTERFACE:include/${ProductName}>
)

target_link_libraries(${ProductName} ws2_32 OpenGL32.lib GLEW::GLEW)

set(SPARKLE_INCLUDE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/include")
set(SPARKLE_LIBRARY_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/x${OSBitness}/${CMAKE_BUILD_TYPE}")

install(TARGETS ${ProductName}
        EXPORT ${ProductName}Targets
        LIBRARY DESTINATION lib/x${OSBitness}/${CMAKE_BUILD_TYPE}
        ARCHIVE DESTINATION lib/x${OSBitness}/${CMAKE_BUILD_TYPE}
        RUNTIME DESTINATION bin/x${OSBitness}/${CMAKE_BUILD_TYPE}
        INCLUDES DESTINATION include
)

install(DIRECTORY src/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
)

install(DIRECTORY src/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
)

install(EXPORT ${ProductName}Targets
        NAMESPACE ${ProductName}::
        DESTINATION lib/cmake/${ProductName}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/${ProductName}Config.cmake.in
                              ${CMAKE_BINARY_DIR}/${ProductName}Config.cmake
                              INSTALL_DESTINATION lib/cmake/${ProductName}
)

install(FILES ${CMAKE_BINARY_DIR}/${ProductName}Config.cmake
        DESTINATION lib/cmake/${ProductName}
)
