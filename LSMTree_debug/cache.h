//
// Created by 魏新鹏 on 2021/4/23.
//

#ifndef LSM_LAB_CACHE_H
#define LSM_LAB_CACHE_H
#include <cstdint>

//class Index {
//public:
//    uint64_t key;
//    uint32_t offset;
//};

class Header {
public:
    uint64_t timeFlag;
    uint64_t size;
    uint64_t max;
    uint64_t min;
    Header(uint64_t timeTmp = 0, uint64_t sizeTmp = 0, uint64_t maxTmp = 0, uint64_t minTmp = 0):timeFlag(timeTmp), size(sizeTmp), max(maxTmp), min(minTmp){}
};

class cache {
public:
    cache(){ bloomFilter = nullptr; keyArray = nullptr; offsetArray = nullptr;}
    ~cache(){ delete keyArray; delete offsetArray; }
    void setHead(Header headTmp) { headerPart = headTmp;}
    void setBloom(bool * bloomTmp) { bloomFilter = bloomTmp;}
    void setKey(uint64_t * keyArrayTmp) { keyArray = keyArrayTmp;}
    void setOffset(uint32_t * offsetArrayTmp) { offsetArray = offsetArrayTmp;}
    bool ifExist(uint64_t key);
    uint64_t binSearch(uint64_t key, uint32_t &length);//return 0 if not find length=string.length()
    uint64_t getTime() const {return headerPart.timeFlag;}
    uint64_t getMax() const {return headerPart.max;}
private:
    /* Header */
    Header headerPart;
    /* bloom */
    bool * bloomFilter;
    /* key array */
    uint64_t * keyArray;
    /* offset array */
    uint32_t * offsetArray;
};

#endif //LSM_LAB_CACHE_H