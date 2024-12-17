project(Playground)

set(PLAYGROUND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PLAYGROUND_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE PLAYGROUND_HEADERS
    ${PLAYGROUND_INCLUDE_DIR}/*.hpp
    ${PLAYGROUND_INCLUDE_DIR}/*.h
)
file(GLOB_RECURSE PLAYGROUND_SOURCES ${PLAYGROUND_SRC_DIR}/*.cpp)

add_executable(Playground ${PLAYGROUND_SOURCES} ${PLAYGROUND_HEADERS})

target_link_libraries(Playground PRIVATE Sparkle)

target_include_directories(Playground
    PRIVATE
        ${PLAYGROUND_INCLUDE_DIR}
        ${SPARKLE_INCLUDE_DIR}
)

set_target_properties(Playground PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin/$<IF:$<CONFIG:Debug>,Debug,Release>/${ARCH_FOLDER}
)
