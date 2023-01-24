//
// Created by 魏新鹏 on 2021/5/31.
//

#ifndef LSMTREE_DEBUG_SSTABLE_H
#define LSMTREE_DEBUG_SSTABLE_H

#include "cache.h"

#include <string>

class SSTable {
 private:
  Cache *cache_;
  char *value_;

 public:
  explicit SSTable(Cache *cache = nullptr,
                   char *value = nullptr)
      : cache_(cache), value_(value) {}
  ~SSTable() { delete value_; }
  Cache *cache() { return cache_; }
  uint64_t cache_size() { return cache_->getSize(); }
  uint64_t getMin() { return cache_->getMin(); }
  uint64_t getMax() { return cache_->getMax(); }
  uint64_t getKey(int index) { return (cache_->key_array())[index]; }
  uint64_t getTime() { return (cache_->header()).time_flag_; }
  std::string getValue(uint64_t index);
  char *getValueArray() { return value_; }
};

#endif//LSMTREE_DEBUG_SSTABLE_H