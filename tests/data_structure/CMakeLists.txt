file(GLOB CURRENT_DIR_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND UNITTEST_SOURCE_FILE ${CURRENT_DIR_CPP_FILES})

set(UNITTEST_SOURCE_FILE "${UNITTEST_SOURCE_FILE}" PARENT_SCOPE)