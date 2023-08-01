# Source files in the src directory
set(SRC_FILES   
    base_entry.cpp
    lru_cache.cpp
    linked_list.cpp
    command.cpp
    consistent-hashing.cpp
    leader.cpp
    worker.cpp
)

add_library(src_lib ${SRC_FILES})
