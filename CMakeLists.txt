cmake_minimum_required(VERSION 3.16)
project(warzone-console-matchmaking VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(warzone_matchmaking
  src/types.cpp
  src/platform.cpp
  src/matcher.cpp
  src/queue.cpp
)

target_include_directories(warzone_matchmaking
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(wz-match src/cli/main.cpp)
target_link_libraries(wz-match PRIVATE warzone_matchmaking)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(warzone_tests tests/matcher_test.cpp)
target_link_libraries(warzone_tests PRIVATE warzone_matchmaking GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(warzone_tests)
