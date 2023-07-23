cmake_minimum_required(VERSION 3.25.2)
project(RedisClone)

set(CMAKE_CXX_STANDARD 20)

option(Tests "Test" OFF) # set with flag -DTest=ON
option(Debug "Debug" OFF) # set with flag -DDebug=ON

if (${Debug})
    set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(include)

add_subdirectory(src)

if (${Tests})
    add_subdirectory(tests)
endif()