file(GLOB CURRENT_DIR_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND SOURCE_FILES ${CURRENT_DIR_CPP_FILES})

add_subdirectory(compilation)
add_subdirectory(glsl_conversion)
add_subdirectory(object)

set(SOURCE_FILES "${SOURCE_FILES}" PARENT_SCOPE)