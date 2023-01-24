add_library(KVStore cache.cpp kvstore.cc sstable.cpp)
target_include_directories(KVStore PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_include_directories(KVStore PUBLIC "${PROJECT_SOURCE_DIR}/include/KVStore")