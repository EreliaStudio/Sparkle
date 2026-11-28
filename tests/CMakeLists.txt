# Register non-overlapping GoogleTest partitions with explicit CTest labels.
# Keep the original test names and individual test processes.
include(GoogleTest)

function(sparkle_discover_category target prefix filter labels)
    string(REPLACE ";" "\\;" escapedLabels "${labels}")
    gtest_discover_tests(${target}
        TEST_PREFIX "${prefix}."
        TEST_FILTER "${filter}"
        PROPERTIES LABELS "${escapedLabels}" ${ARGN}
    )
endfunction()

add_subdirectory(Core)

if(SPARKLE_BUILD_GRAPHICS)
    add_subdirectory(Graphics)
endif()
