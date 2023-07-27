# Source files in the src directory
set(SRC_FILES
    lru_cache.cpp
    linked_list.cpp
    command.cpp
    server.cpp
)

add_library(src_lib ${SRC_FILES})
find_package(cppzmq)

# Create an executable target with main
add_executable(server main.cpp)

# Link src files
target_link_libraries(server cppzmq src_lib)
