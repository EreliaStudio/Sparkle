find_package(GTest REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE SPARKLE_CORE_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Integration/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Stress/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MemorySafety/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Performance/*.cpp"
)

add_executable(SparkleCoreTestSuite
    ${SPARKLE_CORE_TEST_SOURCES}
)

target_compile_features(SparkleCoreTestSuite PRIVATE cxx_std_23)
target_compile_definitions(SparkleCoreTestSuite PRIVATE
    SPARKLE_TEST_RESULTS_DIR="${CMAKE_CURRENT_BINARY_DIR}/results"
    SPARKLE_VOXEL_SHAPES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/voxel/shapes"
)
target_include_directories(SparkleCoreTestSuite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
    ${SPARKLE_SOURCE_DIR}
)
target_link_libraries(SparkleCoreTestSuite
    PRIVATE
        sparkle::core
        GTest::gtest_main
)
set_target_properties(SparkleCoreTestSuite PROPERTIES CXX_EXTENSIONS OFF)

# Explicit suite registration; the complement includes every unclassified test.
# The two lifecycle regressions intentionally run both normally and under ASan.
string(JOIN ":" stressSuites
    "DoubleBufferStressTest.*"
    "EntityHierarchyStressTest.*"
    "NetworkStressTest.*"
    "PublishableStressTest.*"
)
string(JOIN ":" memorySuites
    "EntityHierarchyMemorySafetyTest.*"
    "NetworkMemorySafetyTest.*"
    "PublishableMemorySafetyTest.*"
)
set(lifecycleCases "NetworkConcurrencyTest.ClientDestructor*")
set(repeatConcurrency "NetworkStressTest.ManyClientsConnectAndExchangeConcurrently")
set(repeatRandomized "NetworkStressTest.SeededRandomPayloadsPreserveEveryByte")
set(randomizedScheduler "NetworkStressTest.SeededSchedulerAccountsForRandomPagesAndPriorities")

sparkle_discover_category(SparkleCoreTestSuite Core
    "${repeatConcurrency}" "Core;Stress;RepeatConcurrency")
sparkle_discover_category(SparkleCoreTestSuite Core
    "${repeatRandomized}" "Core;Stress;Randomized;RepeatRandomized")
sparkle_discover_category(SparkleCoreTestSuite Core
    "${randomizedScheduler}" "Core;Stress;Randomized")
sparkle_discover_category(SparkleCoreTestSuite Core
    "${stressSuites}-${repeatConcurrency}:${repeatRandomized}:${randomizedScheduler}" "Core;Stress")
sparkle_discover_category(SparkleCoreTestSuite Core
    "${memorySuites}" "Core;MemorySafety")
sparkle_discover_category(SparkleCoreTestSuite Core
    "${lifecycleCases}" "Core;Fast;Integration;MemorySafety")
sparkle_discover_category(SparkleCoreTestSuite Core
    "PerlinPerformanceTest.*" "Core;Benchmark" RUN_SERIAL TRUE)
sparkle_discover_category(SparkleCoreTestSuite Core
    "*-${stressSuites}:${memorySuites}:${lifecycleCases}:PerlinPerformanceTest.*" "Core;Fast")

