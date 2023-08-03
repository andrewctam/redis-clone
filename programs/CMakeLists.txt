find_package(cppzmq)

# Create an executable for nodes
add_executable(node node.cpp)
target_link_libraries(node cppzmq src_lib)

# Create an executable for server
add_executable(server server.cpp)
target_link_libraries(server cppzmq)

# Create an executable for client
add_executable(client client.cpp)
target_link_libraries(client cppzmq)
