//
// Created by 魏新鹏 on 2021/4/22.
//

#ifndef LSM_LAB_MEMTABLE_H
#define LSM_LAB_MEMTABLE_H

#include "SkipList.h"

class MemTable {
 private:
  SkipList skip_list_;
  bool *bloom_filter_;
  int capacity_;

 public:
  MemTable();
  ~MemTable();
  // bool isFull(uint64_t, uint32_t);
  bool addEntry(uint64_t, const std::string &);
  std::string *search(uint64_t);//return NULL is not exist
  Entry **getWhole();

  bool *getBloom();
  uint64_t getMax();
  uint64_t getMin();
  uint32_t getSize();
  void reset();
  bool empty();
};

#endif//LSM_LAB_MEMTABLE_H