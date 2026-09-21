cmake_minimum_required(VERSION 3.25)
project(SparkleCoreConsumer LANGUAGES CXX)

find_package(sparkle CONFIG REQUIRED COMPONENTS Core)

if(NOT TARGET sparkle::core)
    message(FATAL_ERROR "The Core component did not expose sparkle::core")
endif()
if(TARGET sparkle::sparkle OR sparkle_Graphics_FOUND)
    message(FATAL_ERROR "A Core-only installation unexpectedly exposes Graphics")
endif()

add_executable(core-consumer main.cpp)
target_link_libraries(core-consumer PRIVATE sparkle::core)

enable_testing()
add_test(NAME installed-sparkle-core COMMAND core-consumer)
