#include "cache.h"
#include "MurmurHash3.h"

bool Cache::ifExist(uint64_t key) {
  if (key < header_.min || key > header_.max) return false;

  uint32_t hash[4];
  MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
  for (unsigned int i : hash) {
    if (!bloom_filter_[i % 10240]) return false;
  }
  return true;
}

uint64_t Cache::binSearch(uint64_t key, uint32_t &length) {
  uint64_t left = 0, right = header_.size_;
  while (left < right) {
    uint64_t mid = left + (right - left) / 2;
    if (key_array_[mid] == key) {
      length = offset_array_[mid + 1] - offset_array_[mid];
      return offset_array_[mid];
    } else if (key_array_[mid] > key) {
      right = mid;
    } else {
      left = mid + 1;
    }
  }
  length = 0;
  return 0;
}

bool Cache::addEntry(uint64_t key, const std::string &value) {
  if ((capacity_ = capacity_ - (int) value.size() - 12) <= 0) return false;
  uint32_t hash[4];
  MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
  for (unsigned int i : hash) {
    bloom_filter_[i % 10240] = true;
  }
  return true;
}