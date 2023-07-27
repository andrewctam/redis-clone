cmake_minimum_required(VERSION 3.25.2)
project(RedisClone)

set(CMAKE_CXX_STANDARD 20)

option(TEST "Test" OFF) # set with flag -DTEST=ON
option(DEBUG "Debug" OFF) # set with flag -DDEBUG=ON

if (${DEBUG})
    message("Building for debug")
    set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(include)

add_subdirectory(src)
add_subdirectory(client)

if (${TEST})
    message("Building tests")
    add_subdirectory(tests)
endif()