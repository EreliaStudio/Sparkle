cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(SoloudTryout
	VERSION 1.0
	DESCRIPTION "A template for Project using Sparkle library."
	LANGUAGES CXX)

add_subdirectory("soloud20200207/contrib")

file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(SoloudTryout ${SRC_FILES})
target_include_directories(SoloudTryout PUBLIC "soloud20200207/include")

target_link_directories(SoloudTryout PRIVATE "${CMAKE_SOURCE_DIR}/archive")

target_link_libraries(SoloudTryout Sparkle soloud)