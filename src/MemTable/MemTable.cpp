//
// Created by 魏新鹏 on 2021/4/22.
//

#include "MemTable.h"
#include "MurmurHash3.h"
#include <cstring>

#define DEBUGx
#define DebugCap 34

MemTable::MemTable() {
  bloom_filter_ = new bool[10240]();
  //initial capacity:2MB - 32 - 10240 - 12
  //the last 12 is an useless Index just for calculating the length of last string
  capacity_ = 2086868;
#ifdef DEBUG
  capacity = DebugCap;
#endif
}

MemTable::~MemTable() {
  /* Empty */
}

bool MemTable::addEntry(uint64_t key, const std::string &value) {
  std::string *s;
  if ((s = skip_list_.get(key)) != nullptr) {
    if ((capacity_ = capacity_ + (int) (s->length()) - (int) value.length()) < 0) return false;
  } else {
    if ((capacity_ = capacity_ - (int) value.length() - 12) < 0) return false;
    uint32_t hash[4];
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (unsigned int i : hash) {
      bloom_filter_[i % 10240] = true;
    }
  }
  skip_list_.put(key, value);
  return true;
}

std::string *MemTable::search(uint64_t key) {
  std::string *result = skip_list_.get(key);
  return result;
}

Entry **MemTable::getWhole() {
  return skip_list_.getWhole();
}

bool *MemTable::getBloom() {
  return bloom_filter_;
}

uint64_t MemTable::getMax() {
  return skip_list_.getMax();
}

uint64_t MemTable::getMin() {
  return skip_list_.getMin();
}

uint32_t MemTable::getSize() {
  return skip_list_.size();
}

void MemTable::reset() {
  skip_list_.clear();
  bloom_filter_ = new bool[10240]();
  capacity_ = 2086868;
#ifdef DEBUG
  capacity = DebugCap;
#endif
}

bool MemTable::empty() {
  return skip_list_.empty();
}