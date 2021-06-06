//
// Created by 魏新鹏 on 2021/4/23.
//

#ifndef LSM_LAB_CACHE_H
#define LSM_LAB_CACHE_H
#include <cstdint>
#include <string>
class Header {
public:
    uint64_t timeFlag;
    uint64_t size;
    uint64_t max;
    uint64_t min;
    explicit Header(uint64_t timeTmp = 0, uint64_t sizeTmp = 0, uint64_t maxTmp = 0, uint64_t minTmp = 0):timeFlag(timeTmp), size(sizeTmp), max(maxTmp), min(minTmp){}
};
#define DEBUGx
#define DebugCap 34

class cache {
public:
    cache(){
        capacity = 2086868;
#ifdef DEBUG
        capacity = DebugCap;
#endif
        headerPart = Header();
        bloomFilter = new bool[10240];
        keyArray = nullptr;
        offsetArray = nullptr;
        fileName = -1;
    }
    cache(Header headerTmp, bool* bloomTmp, uint64_t* KeyTmp, uint32_t* offTmp):headerPart(headerTmp),bloomFilter(bloomTmp),keyArray(KeyTmp), offsetArray(offTmp){
        fileName = -1;
    }
    ~cache() { delete keyArray; delete offsetArray; delete bloomFilter;}
    void setHead(Header headTmp) { headerPart = headTmp;}
    Header getHead() {return headerPart;}

    void setBloom(bool * bloomTmp) { bloomFilter = bloomTmp;}
    bool* getBloom() {return bloomFilter;}

    void setKey(uint64_t * keyArrayTmp) { keyArray = keyArrayTmp;}
    uint64_t* getKey() {return keyArray;}

    void setOffset(uint32_t * offsetArrayTmp) { offsetArray = offsetArrayTmp;}
    uint32_t* getOffset() {return offsetArray;}

    bool ifExist(uint64_t key);
    uint64_t binSearch(uint64_t key, uint32_t &length);//return 0 if not find length=string.length()
    uint64_t getTime() const {return headerPart.timeFlag;}
    uint64_t getMax() const {return headerPart.max;}
    uint64_t getMin() const {return headerPart.min;}
    uint64_t getSize() const {return headerPart.size;}
    void setFileName(int filename) { fileName = filename; }
    int getFileName() { return fileName;}
    bool addEntry(uint64_t, const std::string&);
private:
    /* Header */
    Header headerPart;
    /* bloom */
    bool * bloomFilter;
    /* key array */
    uint64_t * keyArray;
    /* offset array */
    uint32_t * offsetArray;

    int fileName;
    int capacity;
};

#endif //LSM_LAB_CACHE_H