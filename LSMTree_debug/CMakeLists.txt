cmake_minimum_required(VERSION 3.17)
project(SkipList_debug)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(memTable)

add_library(Cache cache.cpp)
add_library(SSTable SSTable.cpp)
add_library(KvStore kvstore.cc)

add_executable(correctness correctness.cc)
add_executable(persistence persistence.cc)
add_executable(main main.cpp)

target_link_libraries(persistence SSTable Cache KvStore QuadList SkipList MemTable)
target_link_libraries(correctness SSTable Cache KvStore QuadList SkipList MemTable)
target_link_libraries(main SSTable Cache KvStore QuadList SkipList MemTable)