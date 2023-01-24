#include "SSTable.h"
#include "MurmurHash3.h"

std::string SSTable::getValue(uint64_t index) {
    uint64_t size = cache_size();
    uint64_t begin = (cache_->offset_array())[index] - (32 + 10240 + 12 * (size + 1));
    uint64_t end = (cache_->offset_array())[index + 1] - (32 + 10240 + 12 * (size + 1));
    return std::string(value_ + begin, end - begin);
}