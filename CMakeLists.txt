cmake_minimum_required(VERSION 3.5.0)

project(Sparkle
    VERSION 1.0
    DESCRIPTION "Graphical application library"
    LANGUAGES CXX)	

set(CMAKE_CXX_STANDARD 20)

set(OSBitness 32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OSBitness 64)
endif()

set(ArchitectureSufix "${CMAKE_SYSTEM_NAME}/x${OSBitness}/${CMAKE_BUILD_TYPE}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/archive/${ArchitectureSufix}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ArchitectureSufix}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/executable/${ArchitectureSufix}")

add_subdirectory(Sparkle)

if (BuildPlayground STREQUAL "ON")
	add_subdirectory(Playground)
endif()