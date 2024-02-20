enable_testing()

# Vector2
add_executable(
  vector2_unittest
  vector2_unittest.cpp
)
target_link_libraries(
  vector2_unittest
  GTest::gtest_main
  rapidcheck
  Sparkle
)
target_include_directories(vector2_unittest PRIVATE "${CMAKE_SOURCE_DIR}/includes")
target_link_directories(vector2_unittest PRIVATE "${CMAKE_SOURCE_DIR}/archive")

include(GoogleTest)
gtest_discover_tests(vector2_unittest)

add_test(NAME vector2_unittest COMMAND vector2_unittest)


# Matrix4x4
add_executable(
  matrix4x4_unittest
  matrix4x4_unittest.cpp
)
target_link_libraries(
  matrix4x4_unittest
  GTest::gtest_main
  rapidcheck
  Sparkle
)
target_include_directories(matrix4x4_unittest PRIVATE "${CMAKE_SOURCE_DIR}/includes")
target_link_directories(matrix4x4_unittest PRIVATE "${CMAKE_SOURCE_DIR}/archive")

include(GoogleTest)
gtest_discover_tests(matrix4x4_unittest)

add_test(NAME matrix4x4_unittest COMMAND matrix4x4_unittest)