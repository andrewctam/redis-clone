find_package(cppzmq)

# Create an executable for leader
add_executable(leader_node leader_node.cpp)
target_link_libraries(leader_node cppzmq src_lib)

# Create an executable for worker
add_executable(worker_node worker_node.cpp)
target_link_libraries(worker_node cppzmq src_lib)

# Create an executable for server
add_executable(server server.cpp)
target_link_libraries(server cppzmq)

# Create an executable for client
add_executable(client client.cpp)
target_link_libraries(client cppzmq)
