set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test files
set(TEST_SOURCES
  lru_cache_tests.cpp
  linked_list_tests.cpp
  command_tests.cpp
  consistent_hashing_tests.cpp
)

enable_testing()

# Create test executable
add_executable(tests ${TEST_SOURCES})

find_package(cppzmq)

# Link to src files, cppzmq, and gtest
target_link_libraries(
  tests
  cppzmq
  src_lib 
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
