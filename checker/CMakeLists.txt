enable_testing()

find_package(GTest REQUIRED)

include(GoogleTest)

set(SPARKLE_CHECKER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SPARKLE_CHECKER_SOURCES ${SPARKLE_CHECKER_SRC_DIR}/*.cpp)

# Include directories for tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define the test executable
add_executable(SparkleChecker ${SPARKLE_CHECKER_SOURCES})

# Link libraries
target_link_libraries(SparkleChecker PRIVATE 
    Sparkle
	GTest::GTest GTest::Main GTest::gmock GTest::gmock_main
)

gtest_discover_tests(SparkleChecker)
