cmake_minimum_required(VERSION 3.14)
project(SparkleTUs)

set(TUProductName "SparkleTUs")

include(${CMAKE_SOURCE_DIR}/cmake/importTestLibraries.cmake)

set(OSBitness 32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OSBitness 64)
endif()

set(ArchitectureSufix "${CMAKE_SYSTEM_NAME}/x${OSBitness}/${CMAKE_BUILD_TYPE}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/archive/${ArchitectureSufix}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ArchitectureSufix}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/${ArchitectureSufix}")

importGoogleTest()
importRapidCheck()

enable_testing()

function(create_sparkle_test target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} gtest_main rapidcheck Sparkle)
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/includes")
    target_link_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/archive")
    include(GoogleTest)
    gtest_discover_tests(${target_name})
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

add_subdirectory(data_structure)
add_subdirectory(design_pattern)
add_subdirectory(math)
add_subdirectory(miscellaneous)

foreach(SOURCE_FILE IN LISTS UNITTEST_SOURCE_FILE)
    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
    create_sparkle_test(${TARGET_NAME} ${SOURCE_FILE})
endforeach()

add_executable(${TUProductName} ${UNITTEST_SOURCE_FILE})
target_link_libraries(${TUProductName} gtest_main rapidcheck Sparkle)
target_include_directories(${TUProductName} PRIVATE "${CMAKE_SOURCE_DIR}/includes")
target_link_directories(${TUProductName} PRIVATE "${CMAKE_SOURCE_DIR}/archive")
include(GoogleTest)
gtest_discover_tests(${TUProductName})
add_test(NAME ${TUProductName} COMMAND ${TUProductName})