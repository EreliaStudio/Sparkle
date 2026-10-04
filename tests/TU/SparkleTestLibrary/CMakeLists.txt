find_package(Stb REQUIRED)

add_library(SparkleTestLibrary STATIC
    srcs/image_comparison.cpp
    srcs/open_gl_test_context.cpp
    srcs/paths.cpp
)
add_library(Sparkle::TestLibrary ALIAS SparkleTestLibrary)

target_compile_features(SparkleTestLibrary PUBLIC cxx_std_23)
target_link_libraries(SparkleTestLibrary PUBLIC sparkle::sparkle)
target_include_directories(SparkleTestLibrary
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/includes>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${Stb_INCLUDE_DIR}
)
set_target_properties(SparkleTestLibrary PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME SparkleTestLibrary
    EXPORT_NAME TestLibrary
    DEBUG_POSTFIX d
)

install(TARGETS SparkleTestLibrary
    EXPORT sparkleTestTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY includes/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT sparkleTestTargets
    FILE sparkleTestTargets.cmake
    NAMESPACE Sparkle::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sparkle
)
