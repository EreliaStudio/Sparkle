cmake_minimum_required(VERSION 3.25)
project(SparkleDefaultConsumer LANGUAGES CXX)

find_package(sparkle CONFIG REQUIRED COMPONENTS Core Graphics)

if(NOT TARGET sparkle::core)
    message(FATAL_ERROR "The default installation is missing sparkle::core")
endif()
if(NOT TARGET sparkle::sparkle)
    message(FATAL_ERROR "The default installation is missing sparkle::sparkle")
endif()
if(TARGET Sparkle::TestLibrary OR sparkle_TestLibrary_FOUND)
    message(FATAL_ERROR "The default installation unexpectedly exposes TestLibrary")
endif()

add_executable(default-consumer main.cpp)
target_link_libraries(default-consumer PRIVATE sparkle::sparkle)

enable_testing()
add_test(NAME default-package COMMAND default-consumer)
