
set(INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB CURRENT_DIR_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND SOURCE_FILES ${CURRENT_DIR_CPP_FILES})

add_subdirectory(application)
add_subdirectory(data_structure)
add_subdirectory(external_libraries)
add_subdirectory(design_pattern)
add_subdirectory(game_engine)
add_subdirectory(graphics)
add_subdirectory(input)
add_subdirectory(math)
add_subdirectory(miscellaneous)
add_subdirectory(network)
add_subdirectory(profiler)
add_subdirectory(system)
add_subdirectory(widget)

set(SOURCE_FILES "${SOURCE_FILES}" PARENT_SCOPE)
set(INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}" PARENT_SCOPE)