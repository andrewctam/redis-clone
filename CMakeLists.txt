cmake_minimum_required(VERSION 3.25.2)
project(RedisClone)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

add_subdirectory(src)
add_subdirectory(tests)
