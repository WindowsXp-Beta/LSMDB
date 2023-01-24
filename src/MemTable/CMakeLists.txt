add_library(MemTable MemTable.cpp SkipList.cpp QuadList.cpp)
target_include_directories(MemTable PUBLIC "${PROJECT_SOURCE_DIR}/include/MemTable" "${PROJECT_SOURCE_DIR}/include")