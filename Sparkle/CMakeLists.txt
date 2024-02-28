cmake_minimum_required(VERSION 3.15.2)

set(ProductName "Sparkle")

find_package(GLEW REQUIRED)

add_subdirectory(src)

add_library(${ProductName} ${SOURCE_FILES})

target_include_directories(${ProductName} PUBLIC ${GLEW_INCLUDE_DIRS})
target_include_directories(${ProductName} PUBLIC 
    $<BUILD_INTERFACE:${INCLUDE_DIRECTORIES}>
    $<INSTALL_INTERFACE:include/${ProductName}>
)

target_link_libraries(${ProductName} OpenGL32.lib GLEW::GLEW)

set(SPARKLE_INCLUDE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/include")
set(SPARKLE_LIBRARY_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/x${OSBitness}/${CMAKE_BUILD_TYPE}")

# Specify library and runtime destinations
install(TARGETS ${ProductName}
        EXPORT ${ProductName}Targets
        LIBRARY DESTINATION lib/x${OSBitness}/${CMAKE_BUILD_TYPE}
        ARCHIVE DESTINATION lib/x${OSBitness}/${CMAKE_BUILD_TYPE}
        RUNTIME DESTINATION bin/x${OSBitness}/${CMAKE_BUILD_TYPE}
        INCLUDES DESTINATION include
)

# Install only .hpp files from the src directory
install(DIRECTORY src/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
)

# Export the targets and create the config files
install(EXPORT ${ProductName}Targets
        NAMESPACE ${ProductName}::
        DESTINATION lib/cmake/${ProductName}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/${ProductName}Config.cmake.in
                              ${CMAKE_SOURCE_DIR}/cmake/${ProductName}Config.cmake
                              INSTALL_DESTINATION lib/cmake/${ProductName}
)

install(FILES ${CMAKE_SOURCE_DIR}/${ProductName}Config.cmake
        DESTINATION lib/cmake/${ProductName}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/SparkleConfig.cmake.in
                              "${CMAKE_SOURCE_DIR}/SparkleConfig.cmake"
                              INSTALL_DESTINATION lib/cmake/Sparkle
)