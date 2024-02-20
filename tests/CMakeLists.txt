cmake_policy(SET CMP0135 NEW)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  rapidcheck
  URL https://github.com/emil-e/rapidcheck/archive/ff6af6fc683159deb51c543b065eba14dfcf329b.zip
)
FetchContent_MakeAvailable(rapidcheck)
include_directories(${rapidcheck_SOURCE_DIR}/extras/gtest/include)




target_link_libraries(Playground Sparkle)

add_subdirectory(math)
