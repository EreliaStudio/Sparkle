find_package(GTest REQUIRED)
find_package(Stb REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE SPARKLE_TEST_SUITE_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp
)
file(GLOB_RECURSE SPARKLE_TEST_LIBRARY_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../SparkleTestLibrary/tests/*.cpp
)

add_executable(SparkleTestSuite
    ${SPARKLE_TEST_SUITE_SOURCES}
    ${SPARKLE_TEST_LIBRARY_TEST_SOURCES}
)
target_compile_features(SparkleTestSuite PRIVATE cxx_std_23)
target_link_libraries(SparkleTestSuite
    PRIVATE
        sparkle
        Sparkle::TestLibrary
        GTest::gtest_main
)
target_include_directories(SparkleTestSuite PRIVATE ${Stb_INCLUDE_DIR})
set_target_properties(SparkleTestSuite PROPERTIES CXX_EXTENSIONS OFF)

gtest_discover_tests(SparkleTestSuite
    PROPERTIES
        LABELS unit
)
