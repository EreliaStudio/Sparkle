find_package(GTest REQUIRED)
find_package(Stb REQUIRED)

include(GoogleTest)

file(GLOB_RECURSE SPARKLE_GRAPHICS_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Integration/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Golden/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Performance/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Native/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Stress/*.cpp"
)

set(SPARKLE_CLIPBOARD_TEST_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/Native/core/clipboard_test.cpp
)
list(REMOVE_ITEM SPARKLE_GRAPHICS_TEST_SOURCES
    ${SPARKLE_CLIPBOARD_TEST_SOURCE}
)
file(GLOB_RECURSE SPARKLE_TEST_LIBRARY_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../SparkleTestLibrary/tests/*.cpp
)

add_executable(SparkleGraphicsTestSuite
    helpers/main.cpp
    ${SPARKLE_GRAPHICS_TEST_SOURCES}
    ${SPARKLE_TEST_LIBRARY_TEST_SOURCES}
)

target_compile_features(SparkleGraphicsTestSuite PRIVATE cxx_std_23)
# Template-heavy graphics fixtures can exceed the COFF section limit in MSVC Debug.
if(MSVC)
    target_compile_options(SparkleGraphicsTestSuite PRIVATE /bigobj)
endif()
target_compile_definitions(SparkleGraphicsTestSuite PRIVATE
    SPARKLE_VOXEL_SHAPES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../Core/resources/voxel/shapes"
    SPARKLE_TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    SPARKLE_TEST_RESULTS_DIR="${CMAKE_CURRENT_BINARY_DIR}/results"
)
target_link_libraries(SparkleGraphicsTestSuite
    PRIVATE
        sparkle::sparkle
        Sparkle::TestLibrary
        GTest::gtest
)
target_include_directories(SparkleGraphicsTestSuite PRIVATE
    ${Stb_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/rendering/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/shadow_map
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/voxel
)
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
            LABELS "Graphics\;Native"
            RUN_SERIAL TRUE
    )
endif()

# Only audited context-free suites enter the parallel CPU partition.
# New or unaudited Graphics tests remain in the conservative serial partition.
string(JOIN ":" cpuSuites
    "MesherGeometry.*"
    "MesherTest.*"
    "MesherMaterialIndices.*"
    "ShadowProjectionTest.*"
    "GraphicsColorTest.*"
    "BufferCPUStorageTest.*"
    "MeshTest.*"
    "ColorMesh2DTest.*"
    "TextureMesh2DTest.*"
    "TextureMesh3DTest.*"
    "EventTest.*"
    "MouseMovedEventTest.*"
    "EventDispatcherTest.*"
    "InputStateTest.*"
    "FocusModeTest.*"
    "UpdatableByDeviceTraitTest.*"
    "KeyboardTest.*"
    "MouseTest.*"
)
set(cpuStress "MesherFixtures.*")
set(cpuGolden "PerlinNoiseGolden.*")

string(JOIN ":" goldenSuites
    "BiomeWorldGolden.*"
    "BiomeWorldShadowDiagnostics.*"
    "ShadowRenderingGolden.*"
    "VoxelForwardLightingGolden.*"
    "VoxelTransparencyGolden.*"
    "VoxelTransparencyWindowGolden.*"
    "VoxelViewMaterialGoldenTest.*"
    "VoxelRendererGoldenTest.*"
    "VoxelChunkGoldenTest.*"
    "VoxelShapeGoldenTest.*"
    "VoxelArrangementGoldenTest.*"
    "VoxelCharacterGoldenTest.*"
    "VoxelRoomGoldenTest.*"
    "WidgetGoldenRenderTest.*"
    "WidgetGoldenRenderStateTest.*"
    "Object3DGoldenTest.*"
    "EngineViewModel3DRoomGoldenTest.*"
    "EngineViewFirstImageTest.*"
)

string(JOIN ":" cpuPerformance
    "VoxelChunkPerformanceTest.RandomizedHighExposureCubes16HardBudget"
    "VoxelChunkPerformanceTest.RandomizedMixedShapes16HardBudget"
    "ShadowProjectionPerformance.Randomized128CastersHardBudget"
)
string(JOIN ":" graphicsPerformance
    "ShadowRenderingPerformance.MovingDynamicCasterReusesStaticCascades"
    "BiomeWorldPerformance.DirectionalCascadeDepthRebuildAndCacheHardBudget"
)
string(JOIN ":" benchmarkSuites
    "VoxelChunkPerformanceTest.*"
    "ShadowProjectionPerformance.*"
    "ShadowRenderingPerformance.*"
    "BiomeWorldPerformance.*"
)
set(performanceCases "${cpuPerformance}:${graphicsPerformance}")

sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${cpuSuites}" "Graphics;CPU;Fast")
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${cpuStress}" "Graphics;CPU;Stress")
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${cpuGolden}" "Graphics;CPU;Golden" RESOURCE_LOCK SparkleGraphics)
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${goldenSuites}" "Graphics;Golden" RESOURCE_LOCK SparkleGraphics)
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${cpuPerformance}" "Graphics;CPU;Performance;Randomized" RUN_SERIAL TRUE)
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${graphicsPerformance}" "Graphics;Performance" RUN_SERIAL TRUE)
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "${benchmarkSuites}-${performanceCases}" "Graphics;Benchmark" RUN_SERIAL TRUE)
sparkle_discover_category(SparkleGraphicsTestSuite Graphics
    "*-${cpuSuites}:${cpuStress}:${cpuGolden}:${goldenSuites}:${benchmarkSuites}"
    "Graphics;Functional" RESOURCE_LOCK SparkleGraphics)

