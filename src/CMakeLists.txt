# Source files in the src directory
set(SRC_FILES
    hashmap.cpp
    command.cpp
    server.cpp
    store.cpp
)

# Create an executable target from the source files
add_executable(server ${SRC_FILES})
