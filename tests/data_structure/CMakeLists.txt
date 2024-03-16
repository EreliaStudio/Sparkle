# Collect all C++ files in the current directory
file(GLOB MATH_TEST_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

# Append the collected source files to the global list
set(SourceFiles ${SourceFiles} ${MATH_TEST_SOURCES} PARENT_SCOPE)
