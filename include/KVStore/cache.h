//
// Created by 魏新鹏 on 2021/4/23.
//

#ifndef LSM_LAB_CACHE_H
#define LSM_LAB_CACHE_H

#include <cstdint>
#include <string>

class Header {
 public:
  uint64_t time_flag_;
  uint64_t size_;
  uint64_t max;
  uint64_t min;
  explicit Header(uint64_t time_flag = 0, uint64_t size = 0,
                  uint64_t max = 0, uint64_t min = 0)
      : time_flag_(time_flag), size_(size), max(max), min(min) {}
};

#define DEBUGx
#define DebugCap 34

class Cache {
 public:
  Cache()
      : bloom_filter_(new bool[10240]), key_array_(nullptr),
        offset_array_(nullptr), file_name_(-1) {
    capacity_ = 2086868;
#ifdef DEBUG
    capacity = DebugCap;
#endif
  }

  Cache(Header headerTmp, bool *bloomTmp, uint64_t *KeyTmp, uint32_t *offTmp)
      : header_(headerTmp),
        bloom_filter_(bloomTmp),
        key_array_(KeyTmp),
        offset_array_(offTmp),
        file_name_(-1) {}

  ~Cache() {
    delete key_array_;
    delete offset_array_;
    delete []bloom_filter_;
  }

  void set_header(Header headTmp) { header_ = headTmp; }
  Header header() { return header_; }

  void setBloom(bool *bloomTmp) { bloom_filter_ = bloomTmp; }
  bool *bloom_filter() { return bloom_filter_; }

  void set_key_array(uint64_t *keyArrayTmp) { key_array_ = keyArrayTmp; }
  uint64_t *key_array() { return key_array_; }

  void set_offset_array(uint32_t *offsetArrayTmp) { offset_array_ = offsetArrayTmp; }
  uint32_t *offset_array() { return offset_array_; }

  bool ifExist(uint64_t key);
  uint64_t binSearch(uint64_t key,
                     uint32_t &length);  // return 0 if not find length=string.length()
  uint64_t getTime() const { return header_.time_flag_; }
  uint64_t getMax() const { return header_.max; }
  uint64_t getMin() const { return header_.min; }
  uint64_t getSize() const { return header_.size_; }

  void set_file_name(int filename) { file_name_ = filename; }
  int file_name() const { return file_name_; }
  bool addEntry(uint64_t, const std::string &);

 private:
  /* Header */
  Header header_;
  /* bloom */
  bool *bloom_filter_;
  /* key array */
  uint64_t *key_array_;
  /* offset array */
  uint32_t *offset_array_;

  int file_name_;
  int capacity_;
};

#endif  // LSM_LAB_CACHE_H