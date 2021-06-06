#include "SSTable.h"
#include "MurmurHash3.h"

std::string SSTable::getValue(uint64_t index) {
    uint64_t size = getSize();
    uint64_t begin = (SSTcache -> getOffset())[index] - (32 + 10240 + 12 * (size + 1));
    uint64_t end = (SSTcache -> getOffset())[index + 1] - (32 + 10240 + 12 * (size + 1));
    return std::string(SSTvalue + begin, end - begin);
}