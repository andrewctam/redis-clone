cmake_minimum_required(VERSION 3.25.2)

set(SOURCES
    src/main.cpp
    src/server.cpp
    src/store.cpp
)

project(RedisClone LANGUAGES CXX)


add_executable(${PROJECT_NAME} ${SOURCES})


target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
)
