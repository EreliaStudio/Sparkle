vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

get_filename_component(
    SPARKLE_SOURCE_PATH
    "${CURRENT_PORT_DIR}/../.."
    ABSOLUTE
)

vcpkg_check_features(OUT_FEATURE_OPTIONS SPARKLE_FEATURE_OPTIONS
    FEATURES test-library SPARKLE_BUILD_TEST_LIBRARY
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SPARKLE_SOURCE_PATH}"
    OPTIONS ${SPARKLE_FEATURE_OPTIONS} -DSPARKLE_BUILD_TESTS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME sparkle
    CONFIG_PATH lib/cmake/sparkle
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(
    FILE_LIST "${CURRENT_PORT_DIR}/copyright"
)
