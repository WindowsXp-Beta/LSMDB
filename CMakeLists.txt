cmake_minimum_required(VERSION 3.17)
project(LSMDB)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src/MemTable)
add_subdirectory(src/KVStore)
add_subdirectory(tests)

add_executable(main src/main.cpp)

target_link_libraries(main KVStore MemTable)
target_link_libraries(correctness KVStore MemTable)
target_link_libraries(persistence KVStore MemTable)
target_link_libraries(correctness_random KVStore MemTable)
target_link_libraries(compaction_test KVStore MemTable)