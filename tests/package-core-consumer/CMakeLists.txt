cmake_minimum_required(VERSION 3.25)
project(SparkleCoreConsumer LANGUAGES CXX)

find_package(sparkle CONFIG REQUIRED)

add_executable(core-consumer main.cpp)
target_link_libraries(core-consumer PRIVATE sparkle::sparkle)

enable_testing()
add_test(NAME installed-sparkle-core COMMAND core-consumer)
