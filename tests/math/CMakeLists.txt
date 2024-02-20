enable_testing()

add_executable(
  matrix4_unittest
  matrix4_unittest.cpp
)
target_link_libraries(
  matrix4_unittest
  GTest::gtest_main
  rapidcheck
  Sparkle
)
target_include_directories(matrix4_unittest PRIVATE "${CMAKE_SOURCE_DIR}/includes")
target_link_directories(matrix4_unittest PRIVATE "${CMAKE_SOURCE_DIR}/archive")

include(GoogleTest)
gtest_discover_tests(vector2_unittest)

add_test(NAME matrix4 COMMAND matrix4_unittest)