cmake_minimum_required(VERSION 3.25)
project(SparkleTestConsumer LANGUAGES CXX)

find_package(sparkle CONFIG REQUIRED COMPONENTS Core Graphics TestLibrary)

if(NOT TARGET sparkle::core OR NOT TARGET sparkle::sparkle)
    message(FATAL_ERROR "The graphical package consumer is missing Sparkle targets")
endif()

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE sparkle::sparkle Sparkle::TestLibrary)

enable_testing()
add_test(NAME installed-test-library COMMAND consumer)
