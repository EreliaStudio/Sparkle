find_package(GTest REQUIRED)
find_package(Stb REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE SPARKLE_GRAPHICS_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp
)

set(SPARKLE_CLIPBOARD_TEST_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/srcs/core/clipboard_test.cpp
)
list(REMOVE_ITEM SPARKLE_GRAPHICS_TEST_SOURCES
    ${SPARKLE_CLIPBOARD_TEST_SOURCE}
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
    add_executable(SparkleClipboardTestSuite
        ${SPARKLE_CLIPBOARD_TEST_SOURCE}
        helpers/clipboard_test_main.cpp
    )
    target_compile_features(SparkleClipboardTestSuite PRIVATE cxx_std_23)
    target_include_directories(SparkleClipboardTestSuite PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    target_link_libraries(SparkleClipboardTestSuite
        PRIVATE
            sparkle::sparkle
            GTest::gtest
            user32
    )
    set_target_properties(SparkleClipboardTestSuite PROPERTIES CXX_EXTENSIONS OFF)
    add_dependencies(SparkleGraphicsTestSuite SparkleClipboardTestSuite)

    gtest_discover_tests(SparkleClipboardTestSuite
        TEST_PREFIX "Graphics.Native."
        PROPERTIES
            RUN_SERIAL TRUE
    )
endif()

gtest_discover_tests(SparkleGraphicsTestSuite
    TEST_PREFIX "Graphics."
)
