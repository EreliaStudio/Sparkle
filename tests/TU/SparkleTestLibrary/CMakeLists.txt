find_package(Stb REQUIRED)

add_library(SparkleTestLibrary STATIC
    srcs/image_comparison.cpp
    srcs/paths.cpp
)
add_library(Sparkle::TestLibrary ALIAS SparkleTestLibrary)

target_compile_features(SparkleTestLibrary PUBLIC cxx_std_23)
target_include_directories(SparkleTestLibrary
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
    PRIVATE
        ${Stb_INCLUDE_DIR}
)
target_compile_definitions(SparkleTestLibrary
    PRIVATE
        SPARKLE_TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/resources"
        SPARKLE_TEST_RESULTS_DIR="${CMAKE_CURRENT_BINARY_DIR}/results"
)
set_target_properties(SparkleTestLibrary PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME SparkleTestLibrary
)
