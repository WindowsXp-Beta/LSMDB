#pragma once

#include "cache.h"
#include "SSTable.h"
#include "kvstore_api.h"
#include "../MemTable/MemTable.h"
#include "utils.h"
#include <deque>
#include <fstream>
#include <iostream>
#include <list>

class KVStore : public KVStoreAPI {
  // You can add your implementation here
 public:
  explicit KVStore(const std::string &dir);

  ~KVStore();

  uint32_t getSize() {
    return mem_table_.getSize();
  }

  void put(uint64_t key, const std::string &s) override;

  std::string get(uint64_t key) override;

  bool del(uint64_t key) override;

  void reset() override;

 private:
  MemTable mem_table_;
  uint64_t time_flag_;
  std::deque<std::list<Cache *> *> cache_list_;
  std::string stoDir;
  std::deque<std::vector<int>> slot;//slot用来记录每层的空位

  /* private function */
  std::string getFilePath(uint32_t level, int fileName);
  std::string getFolderPath(uint32_t level);
  SSTable *readSST(uint32_t level, Cache *);
  void writeSST(uint32_t level, int file_name, SSTable *);
  void deleteSST(uint32_t level, int fileName);
  Cache *readCache(std::string &);
  SSTable *createSSTable();
  void compactor(SSTable *ss_table);
};