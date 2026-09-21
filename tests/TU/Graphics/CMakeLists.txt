find_package(GTest REQUIRED)
find_package(Stb REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE SPARKLE_GRAPHICS_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp
)
file(GLOB_RECURSE SPARKLE_TEST_LIBRARY_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../SparkleTestLibrary/tests/*.cpp
)

add_executable(SparkleGraphicsTestSuite
    ${SPARKLE_GRAPHICS_TEST_SOURCES}
    ${SPARKLE_TEST_LIBRARY_TEST_SOURCES}
)

target_compile_features(SparkleGraphicsTestSuite PRIVATE cxx_std_23)
target_compile_definitions(SparkleGraphicsTestSuite PRIVATE
    SPARKLE_TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    SPARKLE_TEST_RESULTS_DIR="${CMAKE_CURRENT_BINARY_DIR}/results"
)
target_link_libraries(SparkleGraphicsTestSuite
    PRIVATE
        sparkle::sparkle
        Sparkle::TestLibrary
        GTest::gtest
)
target_include_directories(SparkleGraphicsTestSuite PRIVATE ${Stb_INCLUDE_DIR})
set_target_properties(SparkleGraphicsTestSuite PROPERTIES CXX_EXTENSIONS OFF)

if(WIN32)
    add_executable(SparkleClipboardHolder helpers/clipboard_holder.cpp)
    target_compile_features(SparkleClipboardHolder PRIVATE cxx_std_23)
    target_link_libraries(SparkleClipboardHolder PRIVATE user32)
    add_dependencies(SparkleGraphicsTestSuite SparkleClipboardHolder)
endif()

gtest_discover_tests(SparkleGraphicsTestSuite
    PROPERTIES
        LABELS "unit;graphics"
)
