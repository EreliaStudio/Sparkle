find_package(GTest REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE SPARKLE_CORE_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp
)

add_executable(SparkleCoreTestSuite
    ${SPARKLE_CORE_TEST_SOURCES}
)

target_compile_features(SparkleCoreTestSuite PRIVATE cxx_std_23)
target_link_libraries(SparkleCoreTestSuite
    PRIVATE
        sparkle::core
        GTest::gtest_main
)
set_target_properties(SparkleCoreTestSuite PROPERTIES CXX_EXTENSIONS OFF)

gtest_discover_tests(SparkleCoreTestSuite
    PROPERTIES
        LABELS "unit;core"
)
