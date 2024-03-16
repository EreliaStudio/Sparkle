cmake_minimum_required(VERSION 3.14)
project(SparkleTUs)

include(${CMAKE_SOURCE_DIR}/cmake/importTestLibraries.cmake)

# # Import testing libraries
importGoogleTest()
importRapidCheck()

enable_testing()

# Function to create a test from a source file
function(create_sparkle_test target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} gtest_main rapidcheck Sparkle)
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/includes")
    target_link_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/archive")
    include(GoogleTest)
    gtest_discover_tests(${target_name})
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

# Add subdirectories here
add_subdirectory(math)
# More add_subdirectory() calls for other directories as needed

# Assuming each subdirectory appends its test sources to SourceFiles
foreach(SOURCE_FILE IN LISTS SourceFiles)
    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
    create_sparkle_test(${TARGET_NAME} ${SOURCE_FILE})
endforeach()