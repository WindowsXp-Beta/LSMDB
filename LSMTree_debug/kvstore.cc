#include "kvstore.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include "MurmurHash3.h"
#include "utils.h"

std::string KVStore::getFilePath(uint32_t level, int fileName) {
    std::stringstream fmt;
    fmt << stoDir << "/level-" << level << "/" << fileName << ".sst";
    return fmt.str();
}

std::string KVStore::getFolderPath(uint32_t level) {
    std::stringstream fmt;
    fmt << stoDir << "/level-" << level;
    return fmt.str();
}

SSTable *KVStore::readSST(uint32_t level, cache* sstCache) {
    int fileName = sstCache -> getFileName();
    std::string resultDir = getFilePath(level, fileName);
    std::ifstream inFile(resultDir, std::ios::in|std::ios::binary);
    if(inFile.fail()) {
        std::cout <<  "Fail open SSTable on level " << level << " fileName is " << fileName << std::endl;
        std::cout.flush();
    }
    uint64_t size = sstCache -> getSize();
    uint32_t valueSize = (sstCache -> getOffset())[size] - (sstCache -> getOffset())[0];
    char * newValueArray = new char [valueSize];
    uint32_t begin = 10272 + (size + 1) * 12;
    inFile.seekg(begin, std::ios::beg);
    inFile.read(newValueArray, valueSize * sizeof(char));
    auto* newSStable = new SSTable(sstCache, newValueArray);
    return newSStable;
}

cache* KVStore::readCache(std::string &fileName) {
    std::ifstream inFile(fileName, std::ios::in|std::ios::binary);
    if(inFile.fail()) {
        std::cout <<  "Fail open SSTable fileName is " << fileName << std::endl;
        std::cout.flush();
    } 
    cache* newCache = new cache();
    /* 读入header */
    uint64_t time;
    inFile.read((char*)&time, sizeof(uint64_t));
    uint64_t size;
    inFile.read((char*)&size, sizeof(uint64_t));
    uint64_t max;
    inFile.read((char*)&max, sizeof(uint64_t));
    uint64_t min;
    inFile.read((char*)&min, sizeof(uint64_t));
    newCache -> setHead(Header(time, size, max, min));

    /* 读入bloomFilter */
    bool * bloomFilter = new bool[10240]();
    inFile.read((char*)bloomFilter, 10240 * sizeof(bool));
    newCache -> setBloom(bloomFilter);

    /* 读入keyArray和offsetArray */
    uint64_t * KeyArray = new uint64_t [size + 1];
    uint32_t * offsetArray = new uint32_t [size + 1];
    inFile.read((char*)KeyArray, (size + 1) * sizeof(uint64_t));
    inFile.read((char*)offsetArray, (size + 1) * sizeof(uint32_t));
    newCache -> setKey(KeyArray);
    newCache -> setOffset(offsetArray);

    return newCache;
}

void KVStore::writeSST(uint32_t level, int fileName, SSTable* ssTable) {
    std::string writeLevel = getFilePath(level, fileName);
    std::ofstream outFile(writeLevel, std::ios::out|std::ios::binary); //ios::out会清除文件中原来的内容
    if(outFile.fail()) {
        std::cout << "Fail open file on level " << level << "and File name is " << fileName;
        std::cout.flush();
    }
    cache* sstCache = ssTable -> getCache();
    Header wHeader = sstCache -> getHead();
    uint64_t size = sstCache -> getSize();
    uint32_t valueSize = (sstCache -> getOffset())[size] - (sstCache -> getOffset())[0];
    outFile.write((char*)(&wHeader), sizeof(wHeader));
    outFile.write((char*)(sstCache -> getBloom()), 10240);
    outFile.write((char*)(sstCache -> getKey()), (size + 1) * sizeof(uint64_t));
    outFile.write((char*)(sstCache -> getOffset()), (size + 1) * sizeof(uint32_t));
    outFile.write((char*)(ssTable -> getValueArray()), valueSize);
}

void KVStore::deleteSST(uint32_t level, int fileName) {
    std::string deleteFileName = getFilePath(level, fileName);
    if(-1 == remove(deleteFileName.data())) {
        std::cout <<  "Fail delete SSTable on level " << level << " fileName is " << fileName << std::endl;
    }
}

/* 从memtable构造SSTable */
SSTable* KVStore::createSSTable() {
    if(memTable.empty()) return nullptr;
    /* Header */
    uint64_t max = memTable.getMax();
    uint64_t min = memTable.getMin();
    uint64_t size = memTable.getSize();
    Header headPart(timeFlag, size, max, min);

    /* Bloom Filter part */
    bool * bloomFilter = memTable.getBloom();

    /* index part */
    uint64_t * newKeyArray = new uint64_t [size + 1];
    uint32_t * newOffsetArray = new uint32_t [size + 1];
    uint32_t offset = 32 + 10240 + 12 * (size + 1);//initial offset
    Entry* *memTableContent = memTable.getWhole();

    int iterator;
    for (iterator = 0; iterator < size; iterator++) {
        newKeyArray[iterator] = memTableContent[iterator] -> key;
        newOffsetArray[iterator] = offset;
        offset += (memTableContent[iterator] -> value.length());
    }
    newOffsetArray[iterator] = offset;
    cache* newCache = new cache(headPart, bloomFilter, newKeyArray, newOffsetArray);

    /* value part */
    uint32_t valueSize = offset - (32 + 10240 + 12 * (size + 1));
    char* newValueArray = new char[valueSize];
    uint32_t cpyStart = 0;
    const char* value;
    for (int i = 0; i < size; i++) {
        value = memTableContent[i] -> value.data();
        memcpy(newValueArray + cpyStart, value, strlen(value));
        cpyStart += strlen(value);
    }
    auto* newSSTable = new SSTable(newCache, newValueArray);
    delete []memTableContent;
    return newSSTable;
}

void KVStore::compactor(SSTable* newSSTable) {
    cache* newCache;
    uint64_t * newKeyArray;
    uint32_t * newOffsetArray;
    char * newValueArray;
    uint32_t cpyStart, valueSize, offset;
    uint64_t largestTime;
    uint64_t max, min, size;
    const char* value;
    int nextLine = 1;
    int iterator;

    /* 初始时，sstList只有第0行的两个sst和将要写入第0行而导致溢出的那个sst */
    std::deque<SSTable*> sstList;
    for (auto& p: (*cacheList[0])){
        sstList.push_back(readSST(0, p));
    }
    sstList.push_back(newSSTable);

    while(true) {
        bool flag = false;//是否在归并时要删除 "~delete~"
        uint64_t minimum = -1, maximum = 0;
        largestTime = 0;
        std::vector<std::list<cache*>::iterator> overlapList;
        for(auto & p : sstList) {
            minimum = p -> getMin() < minimum ? p -> getMin() : minimum; 
            maximum = p -> getMax() > maximum ? p -> getMax() : maximum;
            /* 最大时间戳一定来自上一行 */
            if(p -> getTime() > largestTime) largestTime = p -> getTime();
        }
        if(nextLine < cacheList.size()) {//nextLine exist
            if(nextLine == cacheList.size() - 1) flag = true;//如果下一行是最后一行，打开flag
            for(auto it = (cacheList[nextLine] -> begin()); it != (cacheList[nextLine] -> end()); it++) {
                if(!((*it) -> getMax() < minimum || (*it) -> getMin() > maximum)) overlapList.push_back(it);
            }
            for (auto &p: overlapList) {
                sstList.push_back(readSST(nextLine, (*p)));
            }
        }
        if(!overlapList.empty()) std::cout << "compaction" << std::endl;
        else if(nextLine == 1) {//下一行不存在且1是下一行也要开启flag
            flag = true;
        }
        /* begin compaction */
        /* 最后将所有sstable归并成一个大的entry数组 */
        std::vector<Entry> entryList;
        int *pointer = new int[sstList.size()]();//pointer数组保存每个sstable遍历到第几个元素
        while(true) {
            uint64_t smallest = -1, maxTimeFlag = 0;
            std::list<int> ownerList;//记录当前相同最小key的所有SSTable
            int owner = -1;//记录当前最小key的所有SSTable中的时间戳最大的
            for(int i = 0; i < sstList.size(); i++) {
                if(pointer[i] < 0) continue;//pointer[i]等于-1意味着其已经遍历完成
                uint64_t smallTmp = sstList[i] -> getKey(pointer[i]);
                if(smallTmp < smallest) {
                    ownerList.clear();
                    ownerList.push_back(i);
                    owner = i;
                    smallest = smallTmp;
                    maxTimeFlag = sstList[i] -> getTime();
                }
                else if(smallest == smallTmp) {
                    if(sstList[i] -> getTime() > maxTimeFlag) {
                        owner = i;
                        maxTimeFlag = sstList[i] -> getTime();
                    }
                    ownerList.push_back(i);
                }
            }
            if(owner < 0) break;//归并完成
            std::string value = sstList[owner] -> getValue(pointer[owner]);
            for(auto & p:ownerList) {
                pointer[p]++;
                if (pointer[p] == (sstList[p]->getSize())) {//if reach size
                    pointer[p] = -1;
                }
            }
            if(flag) {
                if(value == "~DELETED~") continue;
            }
            entryList.emplace_back(smallest, value);
        }
        delete []pointer;
        /* 释放SSTable */
        while(!sstList.empty()) {
            delete sstList.back();
            sstList.pop_back();
        }
        /* 删除cache和disk上的SSTable */
        for(auto &p: overlapList) {
            int fileName = (*p) -> getFileName();
            slot[nextLine].push_back(fileName);
            delete *p;
            cacheList[nextLine] -> erase(p);
            deleteSST(nextLine, fileName);
        }

        /* rebuild SSTable */
        for(int i = 0; i < entryList.size();) {
            newCache = new cache();
            size = 0;
            min = i;//i为entryList中的下标，min用来找到最小key
            while(newCache -> addEntry(entryList[i].key, entryList[i].value) && i < entryList.size()) {
                i++;
                size++;
            }
            max = i - 1;//因为i没有加到这个cache里，下一个cache的min就从i开始
            newCache -> setHead(Header(largestTime, size, entryList[max].key, entryList[min].key));

            newKeyArray = new uint64_t[size + 1];
            newOffsetArray = new uint32_t[size + 1];
            offset = 32 + 10240 + 12 * (size + 1);//initial offset
            for (iterator = 0; iterator < size; iterator++){
                newKeyArray[iterator] = entryList[min + iterator].key;
                newOffsetArray[iterator] = offset;
                offset += entryList[iterator + min].value.length();
            }
            newOffsetArray[iterator] = offset;
            newCache -> setKey(newKeyArray);
            newCache -> setOffset(newOffsetArray);

            valueSize = offset - (32 + 10240 + 12 * (size + 1));
            newValueArray = new char[valueSize]();
            cpyStart = 0;
            for(iterator = min; iterator <= max; iterator++) {
                value = entryList[iterator].value.data();
                memcpy(newValueArray + cpyStart, value, strlen(value));
                cpyStart += strlen(value);
            }
            newSSTable = new SSTable(newCache, newValueArray);
            sstList.push_front(newSSTable);
        }

        /* write SSTable and update cache */
        /* 如果下一层空，直接顺序写入 */
        if(nextLine >= cacheList.size()) {
            std::string newLevel = getFolderPath(nextLine);
            utils::mkdir(newLevel.data());
            auto* newCacheList = new std::list<cache*>;
            cacheList.push_back(newCacheList);
            slot.emplace_back(std::vector<int>());
            for(int i = 0; i < sstList.size(); i++) {
                cache* writeCache = sstList[i] -> getCache();
                writeCache -> setFileName(i);
                newCacheList -> push_back(writeCache);
                writeSST(nextLine, i, sstList[i]);
            }
            while(!sstList.empty()) {
                delete sstList.back();//删除SSTable
                sstList.pop_back();
            }
            break;//finish compaction
        }
        /* 下一层能容纳 */
        else if( (cacheList[nextLine] -> size() + sstList.size()) <= (2 << nextLine) ) {
            int i = 0;
            for(;!slot[nextLine].empty() && i < sstList.size(); i++) {//先填slot
                int fileName = slot[nextLine].back();
                slot[nextLine].pop_back();
                cache* writeCache = sstList[i] -> getCache();
                writeCache -> setFileName(fileName);
                writeSST(nextLine, fileName, sstList[i]);
                cacheList[nextLine] -> push_back(writeCache);
            }
            for(; i < sstList.size(); i++) {//slot填满，直接插在list尾
                int fileName = cacheList[nextLine] -> size();
                cache* writeCache = sstList[i] -> getCache();
                writeCache -> setFileName(fileName);
                writeSST(nextLine, fileName, sstList[i]);
                cacheList[nextLine] -> push_back(writeCache);
            }
            /* 删除sstlist */
            while(!sstList.empty()) {
                delete sstList.back();
                sstList.pop_back();
            }
            break;
        }
        /* 下一层容纳不下，即要多次归并 */
        else {
            size_t writeSize = sstList.size();
            /* 下一层容量大于要写入的容量 */
            if(writeSize < (2 << nextLine)) {
                int popOut = cacheList[nextLine] -> size() - ((2 << nextLine) - writeSize);
                /* 找出要被挤走的SSTable */
                for(int i = 0; i < popOut; i++) {
                    uint64_t minTimeStamp = -1, minKey = -1;//当前遍历到的最小时间戳，最小键
                    std::list<cache*>::iterator minPointer;//最小时间戳and最小键对应的迭代器
                    for(auto p = cacheList[nextLine] -> begin(); p != cacheList[nextLine] -> end(); p++){
                        if((*p) -> getTime() < minTimeStamp) {
                            minTimeStamp = (*p) -> getTime();
                            minKey = (*p) -> getMin();
                            minPointer = p;
                        }
                        else if((*p) -> getTime() == minTimeStamp) {
                            if((*p) -> getMin() < minKey){
                                minKey = (*p) -> getMin();
                                minPointer = p;
                            }
                        }
                    }
                    sstList.push_back(readSST(nextLine, (*minPointer)));
                    slot[nextLine].push_back((*minPointer) -> getFileName());
                    cacheList[nextLine] -> erase(minPointer);//从链表中删除该cache，注意堆中仍保留
                }
                /* 将sstlist的writeSize个写入下一层 */
                /* 注意，此时sstlist的末尾已经是这一行被挤出的了 */
                int i = 0;
                for(; !slot[nextLine].empty() && i < writeSize; i++) {//先写slot
                    int fileName = slot[nextLine].back();
                    slot[nextLine].pop_back();
                    cache* writeCache = sstList[i] -> getCache();
                    writeCache -> setFileName(fileName);
                    writeSST(nextLine, fileName, sstList[i]);
                    cacheList[nextLine] -> push_back(writeCache);
                }
                for(; i < writeSize; i++) {//slot填满
                    int fileName = cacheList[nextLine] -> size();
                    cache* writeCache = sstList[i] -> getCache();
                    writeCache -> setFileName(fileName);
                    writeSST(nextLine, fileName, sstList[i]);
                    cacheList[nextLine] -> push_back(writeCache);
                }
                /* 删除sstlist */
                for(int i = 0; i < writeSize; i++) {
                    delete sstList.front();
                    sstList.pop_front();
                }
            }
            /* 下一层容量小于等于要写入的容量 */
            else {
                /* 将下一层所有sstable读入 */
                // for(auto p = cacheList[nextLine] -> begin(); p != cacheList[nextLine] -> end(); p++) {
                //     sstList.push_back(readSST(nextLine, (*p)));
                //     cacheList[nextLine] -> erase(p);
                // }
                while(!cacheList[nextLine] -> empty()) {
                    cache* readInCache = cacheList[nextLine] -> back();
                    sstList.push_back(readSST(nextLine, readInCache));
                    cacheList[nextLine] -> pop_back();
                }
                slot[nextLine].clear();//清空slot
                /* sstlist写 2<<nextLine 个到下一行 */
                writeSize = 2 << nextLine;
                for(int i = 0; i < writeSize; i++) {
                    cache* writeCache = sstList[i] -> getCache();
                    writeCache -> setFileName(i);
                    cacheList[nextLine] -> push_back(writeCache);
                    writeSST(nextLine, i, sstList[i]);
                }
                /* 删除sstlist */
                for(int i = 0; i < writeSize; i++) {
                    delete sstList.front();
                    sstList.pop_front();
                }
            }
        }
        nextLine++;
    }
    /* 此时要将第0行的cache和sstable清除 */
    // for(auto p = cacheList[0] -> begin(); p != cacheList[0] -> end(); p++) {
    //     deleteSST(0, (*p) -> getFileName());
    //     delete *p;
    //     cacheList[0] -> erase(p);
    // }
    while(!cacheList[0] -> empty()) {
        cache* deleteCache = cacheList[0] -> back();
        deleteSST(0, deleteCache -> getFileName());
        delete deleteCache;
        cacheList[0] -> pop_back();
    }
}

KVStore::KVStore(const std::string &dir): KVStoreAPI(dir)
{
    stoDir = dir;
    timeFlag = 0;
    /* 从已有sstable中构造初始的cache */
    std::vector<std::string> folderList;
    int folderNum = utils::scanDir(stoDir, folderList);
    /* 返回的folder是倒着的 level-n level-(n-1) ... level-0 */
    for(int i = 0; i < folderNum; i++) {
        auto *newCacheList = new std::list< cache* >;
        cacheList.push_back(newCacheList);
        std::string folder = stoDir + "/" + folderList[i];
        std::vector<std::string> fileList;
        int fileNum = utils::scanDir(folder, fileList);
        bool *slotFlag = new bool[2 << i]();
        int maxFileName = -1;
        for(int j = 0; j < fileNum; j++) {
            std::string fileName = folder + "/" + fileList[j];
            cache* newCache = readCache(fileName);
            int filename = atoi(fileList[j].data());
            slotFlag[filename] = true;
            maxFileName = filename > maxFileName ? filename : maxFileName;
            timeFlag = newCache -> getTime() > timeFlag ? newCache -> getTime() : timeFlag;
            newCache -> setFileName(filename);
            newCacheList -> push_back(newCache);
        }
        /* 构造slot */
        slot.emplace_back(std::vector<int>());
        for(int j = 0; j <= maxFileName; j++) {
            if(!slotFlag[j]) slot[i].push_back(j);
        }
        delete []slotFlag;
    }
    timeFlag++;
    if(folderNum == 0) {//如果没有folder则要构造第0行
        std::string level0 = stoDir + "/level-0";
        utils::mkdir(level0.data());
        auto *listZero = new std::list< cache* >;//level0's cache
        cacheList.push_back(listZero);
        slot.emplace_back(std::vector<int>());
    }
}

KVStore::~KVStore()
{
    /* 将存在内存中的memtable生成sstable，写入disk */
    SSTable* newSSTable = createSSTable();
    if(newSSTable) {
        cache *newCache = newSSTable -> getCache();
        /* begin write sstable */
        if (cacheList[0]->size() < 2) {//dont need compaction
            int fileName = cacheList[0] -> size();
            newCache -> setFileName(fileName);
            writeSST(0, fileName, newSSTable);
            cacheList[0] -> push_back(newCache);
            delete newSSTable;
        }
        /* need compaction */
        else compactor(newSSTable);
    }
    /* 删除所有cache */
    while(!cacheList.empty()){
        for(auto &p: *cacheList.back()) {
            delete p;
        }
        delete cacheList.back();
        cacheList.pop_back();
    }
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
    if (!memTable.addEntry(key, s)) {
        SSTable* newSSTable = createSSTable();
        cache* newCache = newSSTable -> getCache();
        /* if there is no cacheList[0] it happens when after reset() */
        if( cacheList.empty() ) {
            std::string level0 = stoDir + "/level-0";
            utils::mkdir(level0.data());
            auto *listZero = new std::list< cache* >;//level0's cache
            cacheList.push_back(listZero);
            slot.emplace_back(std::vector<int>());
        }
        /* begin write sstable */
        if(cacheList[0] -> size() < 2) {//dont need compaction
            //<del>新的cache直接插到尾部，fileName为cacheList[0]的size</del>
            //新的cache先插slot
            int fileName;
            if(!slot[0].empty()) {
                fileName = slot[0].back();
                slot[0].pop_back();
            }
            else {
                fileName = cacheList[0] -> size();
            }
            newCache -> setFileName(fileName);
            writeSST(0, fileName, newSSTable);
            cacheList[0] -> push_back(newCache);
            delete newSSTable;
        }
        /* need compaction */
        else compactor(newSSTable);

        timeFlag++;
        memTable.reset();
        memTable.addEntry(key, s);
    }
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
    std::string *ret_str_p;
    if ((ret_str_p = memTable.search(key)) != nullptr) {//find in memTable
        if (*ret_str_p == "~DELETE~") return "";
        else return *ret_str_p;
    }
    else { //search in cache if find goto disk to get it
        int fileName = -1;
        uint64_t bigTime = 0;
        uint32_t offset = 0, length = 0, finalLength = 0;
        uint32_t level = 0;
        for (int i = 0; i < cacheList.size(); i++) {
            auto p = cacheList[i] -> begin();
            for(;p != cacheList[i] -> end(); p++) {//遍历一层
                if ((*p) -> ifExist(key)) {//在cache中存在
                    uint64_t offsetTmp = (*p) -> binSearch(key, length);//二分搜索
                    if ( offsetTmp != 0) {//二分找到
                        uint64_t timeTmp = (*p) -> getTime();
                        if (timeTmp > bigTime) {//如果时间戳更新
                            offset = offsetTmp;
                            bigTime = timeTmp;
                            level = i;
                            fileName = (*p) -> getFileName();
                            finalLength = length;
                        }
                    }
                }
            }
        }

        if(fileName == -1) return "";//if not find

        std::string resultDir = getFilePath(level, fileName);
        std::ifstream inFile(resultDir, std::ios::in|std::ios::binary);
        inFile.seekg(offset, std::ios::beg);
        char *ret_str_char = new char[finalLength + 1]();
        inFile.read(ret_str_char, finalLength);
        std::string ret_str(ret_str_char);
        if (ret_str == "~DELETE~") ret_str.clear();
        delete []ret_str_char;
        return ret_str;
    }
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
    std::string result = get(key);
    if (!result.empty()) {
        put(key, "~DELETE~");
        return true;
    }
    else return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset(){
    /* 删除SSTable和cache */
    for(int i = 0; i < cacheList.size(); i++) {
        auto list = cacheList[i];
        while(!list -> empty()) {
            cache* deleteCache = list -> back();
            int fileName = deleteCache -> getFileName();
            deleteSST(i, fileName);
            delete deleteCache;
            list -> pop_back();
        }
        if(-1 == utils::rmdir(getFolderPath(i).data())) {
            std::cout << "Cannot delete folder level " << i << std::endl;
        }
    }
    /* 清空cacheList */
    while(!cacheList.empty()) {
        cacheList.pop_back();
    }
    /* 清空memTable */
    memTable.reset();
    /* 清空slot */
    while(!slot.empty()){
        slot.pop_back();
    }
    timeFlag = 0;
}

