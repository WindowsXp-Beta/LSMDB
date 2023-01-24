#include "kvstore.h"
#include "MurmurHash3.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#define PARTcachex
#define NOcachex

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

SSTable *KVStore::readSST(uint32_t level, Cache *cache) {
  int file_name = cache->file_name();
  std::string result_dir = getFilePath(level, file_name);
  std::ifstream inFile(result_dir, std::ios::in | std::ios::binary);
  if (inFile.fail()) {
    std::cout << "Fail open SSTable on level " << level << " fileName is " << file_name << std::endl;
    std::cout.flush();
  }
  uint64_t size = cache->getSize();
  uint32_t value_size = (cache->offset_array())[size] - (cache->offset_array())[0];
  char *value_array = new char[value_size];
  uint32_t begin = 10272 + (size + 1) * 12;
  inFile.seekg(begin, std::ios::beg);
  inFile.read(value_array, value_size * sizeof(char));
  inFile.close();
  return new SSTable(cache, value_array);
}

Cache *KVStore::readCache(std::string &fileName) {
  std::ifstream inFile(fileName, std::ios::in | std::ios::binary);
  if (inFile.fail()) {
    std::cout << "Fail open SSTable fileName is " << fileName << std::endl;
    std::cout.flush();
  }
  Cache *newCache = new Cache();
  /* 读入header */
  uint64_t time;
  inFile.read((char *) &time, sizeof(uint64_t));
  uint64_t size;
  inFile.read((char *) &size, sizeof(uint64_t));
  uint64_t max;
  inFile.read((char *) &max, sizeof(uint64_t));
  uint64_t min;
  inFile.read((char *) &min, sizeof(uint64_t));
  newCache->set_header(Header(time, size, max, min));

  /* 读入bloomFilter */
  bool *bloomFilter = new bool[10240]();
  inFile.read((char *) bloomFilter, 10240 * sizeof(bool));
  newCache->setBloom(bloomFilter);

  /* 读入keyArray和offsetArray */
  uint64_t *KeyArray = new uint64_t[size + 1];
  uint32_t *offsetArray = new uint32_t[size + 1];
  inFile.read((char *) KeyArray, (size + 1) * sizeof(uint64_t));
  inFile.read((char *) offsetArray, (size + 1) * sizeof(uint32_t));
  newCache->set_key_array(KeyArray);
  newCache->set_offset_array(offsetArray);

  return newCache;
}

void KVStore::writeSST(uint32_t level, int file_name, SSTable *ssTable) {
  std::string write_level = getFilePath(level, file_name);
  std::ofstream out_file(write_level, std::ios::out | std::ios::binary); //ios::out会清除文件中原来的内容
  if (out_file.fail()) {
    std::cout << "Fail open file on level " << level << " and File name is " << file_name;
    std::cout.flush();
  }
  Cache *sstCache = ssTable->cache();
  Header wHeader = sstCache->header();
  uint64_t size = sstCache->getSize();
  uint32_t valueSize = (sstCache->offset_array())[size] - (sstCache->offset_array())[0];
  out_file.write((char *) (&wHeader), sizeof(wHeader));
  out_file.write((char *) (sstCache->bloom_filter()), 10240);
  out_file.write((char *) (sstCache->key_array()), (size + 1) * sizeof(uint64_t));
  out_file.write((char *) (sstCache->offset_array()), (size + 1) * sizeof(uint32_t));
  out_file.write((char *) (ssTable->getValueArray()), valueSize);
  out_file.close();
}

void KVStore::deleteSST(uint32_t level, int fileName) {
  std::string file_name = getFilePath(level, fileName);
  if (-1 == remove(file_name.data())) {
    std::cout << "Fail delete SSTable on level " << level << " fileName is " << fileName << std::endl;
  }
}

/* 从memtable构造SSTable */
SSTable *KVStore::createSSTable() {
  if (mem_table_.empty()) return nullptr;
  /* Header */
  uint64_t max = mem_table_.getMax();
  uint64_t min = mem_table_.getMin();
  uint64_t size = mem_table_.getSize();
  Header header(time_flag_, size, max, min);

  /* Bloom Filter part */
  bool *bloom_filter = mem_table_.getBloom();

  /* index part */
  uint64_t *new_key_array = new uint64_t[size + 1]; //TODO: key_array's size can be size
  uint32_t *new_offset_array = new uint32_t[size + 1];
  uint32_t offset = 32 + 10240 + 12 * (size + 1); //initial offset
  Entry **mem_table_content = mem_table_.getWhole();

  int it;
  for (it = 0; it < size; it++) {
    new_key_array[it] = mem_table_content[it]->key;
    new_offset_array[it] = offset;
    offset += (mem_table_content[it]->value.length());
  }
  new_offset_array[it] = offset;
  Cache *new_cache = new Cache(header, bloom_filter, new_key_array, new_offset_array);

  /* value part */
  uint32_t value_size = offset - (32 + 10240 + 12 * (size + 1));
  char *new_value_array = new char[value_size];
  uint32_t copy_start = 0;
  const char *value;
  for (int i = 0; i < size; i++) {
    value = mem_table_content[i]->value.data();
    memcpy(new_value_array + copy_start, value, strlen(value));
    copy_start += strlen(value);
  }
  SSTable *new_SSTable = new SSTable(new_cache, new_value_array);
  delete []mem_table_content;
  return new_SSTable;
}

void KVStore::compactor(SSTable *ss_table) {
  Cache *cache;
  uint64_t *key_array;
  uint32_t *offset_array;
  char *value_array;
  uint32_t copy_start, value_size, offset;
  uint64_t largest_time;
  uint64_t max, min, size;
  const char *value;
  int next_line = 1;
  int iterator;

  /* 初始时，sstList只有第0行的两个sst和将要写入第0行而导致溢出的那个sst */
  std::deque<SSTable *> sst_list; //stores all the sstable need to be merged
  for (auto &p : (*cache_list_[0])) {
    sst_list.push_back(readSST(0, p));
  }
  sst_list.push_back(ss_table);

  while (true) {
    bool flag = false;  //是否在归并时要删除 "~delete~"
    uint64_t minimum = -1, maximum = 0;
    largest_time = 0;
    std::vector<std::list<Cache *>::iterator> overlap_list;
    for (auto &sst : sst_list) {
      minimum = sst->getMin() < minimum ? sst->getMin() : minimum;
      maximum = sst->getMax() > maximum ? sst->getMax() : maximum;
      /* 最大时间戳一定来自上一行 */
      if (sst->getTime() > largest_time) largest_time = sst->getTime();
    }
    if (next_line < cache_list_.size()) { //nextLine exist
      if (next_line == cache_list_.size() - 1) flag = true; //如果下一行是最后一行，打开flag
      for (auto it = (cache_list_[next_line]->begin()); it != (cache_list_[next_line]->end()); it++) {
        if (!((*it)->getMax() < minimum || (*it)->getMin() > maximum)) overlap_list.push_back(it);
      }
      for (auto &p : overlap_list) {
        sst_list.push_back(readSST(next_line, (*p)));
      }
    }
//    if(!overlapList.empty()) std::cout << "compaction" << std::endl;
    else if (next_line == 1) {  //下一行不存在且1是下一行也要开启flag
      flag = true;
    }
    /* begin compaction */
    /* 最后将所有sstable归并成一个大的entry数组 */
    std::vector<Entry> entry_list;
    int *pointer = new int[sst_list.size()]();  //pointer数组保存每个sstable遍历到第几个元素
    while (true) {
      uint64_t smallest = -1, max_time_flag = 0;
      std::list<int> owner_list;//记录当前相同最小key的所有SSTable
      int owner = -1;          //记录当前最小key的所有SSTable中的时间戳最大的
      for (int i = 0; i < sst_list.size(); i++) {
        if (pointer[i] < 0) continue; //pointer[i]等于-1意味着其已经遍历完成
        uint64_t small_tmp = sst_list[i]->getKey(pointer[i]);
        if (small_tmp < smallest) {
          owner_list.clear();
          owner_list.push_back(i);
          owner = i;
          smallest = small_tmp;
          max_time_flag = sst_list[i]->getTime();
        } else if (smallest == small_tmp) {
          if (sst_list[i]->getTime() > max_time_flag) {
            owner = i;
            max_time_flag = sst_list[i]->getTime();
          }
          owner_list.push_back(i);
        }
      }
      if (owner < 0) break; //归并完成
      std::string chosen_value = sst_list[owner]->getValue(pointer[owner]);
      for (int p : owner_list) {
        pointer[p]++;
        if (pointer[p] == (sst_list[p]->cache_size()))  //if reach size
          pointer[p] = -1;
      }
      if (flag && chosen_value == "~DELETED~") continue;
      entry_list.emplace_back(smallest, chosen_value);
    }
    delete []pointer;
    /* 释放SSTable */
    while (!sst_list.empty()) {
      delete sst_list.back();
      sst_list.pop_back();
    }
    /* 删除cache和disk上的SSTable */
    for (auto &cache_it : overlap_list) {
      int file_name = (*cache_it)->file_name();
      slot[next_line].push_back(file_name);
      delete *cache_it;
      cache_list_[next_line]->erase(cache_it);
      deleteSST(next_line, file_name);
    }

    /* rebuild SSTable */
    for (int i = 0; i < entry_list.size();) {
      cache = new Cache();
      size = 0;
      min = i;//i为entryList中的下标，min用来找到最小key
      while (cache->addEntry(entry_list[i].key, entry_list[i].value) && i < entry_list.size()) {
        i++;
        size++;
      }
      max = i - 1;//因为i没有加到这个cache里，下一个cache的min就从i开始
      cache->set_header(Header(largest_time, size, entry_list[max].key, entry_list[min].key));

      key_array = new uint64_t[size + 1];
      offset_array = new uint32_t[size + 1];
      offset = 32 + 10240 + 12 * (size + 1);//initial offset
      for (iterator = 0; iterator < size; iterator++) {
        key_array[iterator] = entry_list[min + iterator].key;
        offset_array[iterator] = offset;
        offset += entry_list[iterator + min].value.length();
      }
      offset_array[iterator] = offset;
      cache->set_key_array(key_array);
      cache->set_offset_array(offset_array);

      value_size = offset - (32 + 10240 + 12 * (size + 1));
      value_array = new char[value_size]();
      copy_start = 0;
      for (iterator = min; iterator <= max; iterator++) {
        value = entry_list[iterator].value.data();
        memcpy(value_array + copy_start, value, strlen(value));
        copy_start += strlen(value);
      }
      ss_table = new SSTable(cache, value_array);
      sst_list.push_front(ss_table);
    }

    /* write SSTable and update cache */
    /* 如果下一层空，直接顺序写入 */
    if (next_line >= cache_list_.size()) {
      std::string new_level = getFolderPath(next_line);
      utils::mkdir(new_level.data());
      auto *cache_list = new std::list<Cache *>;
      cache_list_.push_back(cache_list);
      slot.emplace_back();
      for (int i = 0; i < sst_list.size(); i++) {
        Cache *write_cache = sst_list[i]->cache();
        write_cache->set_file_name(i);
        cache_list->push_back(write_cache);
        writeSST(next_line, i, sst_list[i]);
      }
      while (!sst_list.empty()) {
        delete sst_list.back();//删除SSTable
        sst_list.pop_back();
      }
      break;//finish compaction
    }
    /* 下一层能容纳 */
    else if ((cache_list_[next_line]->size() + sst_list.size()) <= (2 << next_line)) {
      int i = 0;
      for (; !slot[next_line].empty() && i < sst_list.size(); i++) {  //先填slot
        int file_name = slot[next_line].back();
        slot[next_line].pop_back();
        Cache *write_cache = sst_list[i]->cache();
        write_cache->set_file_name(file_name);
        writeSST(next_line, file_name, sst_list[i]);
        cache_list_[next_line]->push_back(write_cache);
      }
      for (; i < sst_list.size(); i++) {  //slot填满，直接插在list尾
        int file_name = cache_list_[next_line]->size();
        Cache *write_cache = sst_list[i]->cache();
        write_cache->set_file_name(file_name);
        writeSST(next_line, file_name, sst_list[i]);
        cache_list_[next_line]->push_back(write_cache);
      }
      /* 删除sstlist */
      while (!sst_list.empty()) {
        delete sst_list.back();
        sst_list.pop_back();
      }
      break;
    }
    /* 下一层容纳不下，即要多次归并 */
    else {
      size_t write_size = sst_list.size();
      /* 下一层容量大于要写入的容量 */
      if (write_size < (2 << next_line)) {
        int pop_out = cache_list_[next_line]->size() - ((2 << next_line) - write_size);
        /* 找出要被挤走的SSTable */
        for (int i = 0; i < pop_out; i++) {
          uint64_t min_time_stamp = -1, min_key = -1;  //当前遍历到的最小时间戳，最小键
          std::list<Cache *>::iterator min_pointer;  //最小时间戳and最小键对应的迭代器
          for (auto p = cache_list_[next_line]->begin(); p != cache_list_[next_line]->end(); p++) {
            if ((*p)->getTime() < min_time_stamp) {
              min_time_stamp = (*p)->getTime();
              min_key = (*p)->getMin();
              min_pointer = p;
            } else if ((*p)->getTime() == min_time_stamp) {
              if ((*p)->getMin() < min_key) {
                min_key = (*p)->getMin();
                min_pointer = p;
              }
            }
          }
          sst_list.push_back(readSST(next_line, (*min_pointer)));
          slot[next_line].push_back((*min_pointer)->file_name());
          cache_list_[next_line]->erase(min_pointer);//从链表中删除该cache，注意堆中仍保留
        }
        /* 将sstlist的writeSize个写入下一层 */
        /* 注意，此时sstlist的末尾已经是这一行被挤出的了 */
        int i = 0;
        for (; !slot[next_line].empty() && i < write_size; i++) {//先写slot
          int fileName = slot[next_line].back();
          slot[next_line].pop_back();
          Cache *writeCache = sst_list[i]->cache();
          writeCache->set_file_name(fileName);
          writeSST(next_line, fileName, sst_list[i]);
          cache_list_[next_line]->push_back(writeCache);
        }
        for (; i < write_size; i++) {//slot填满
          int fileName = cache_list_[next_line]->size();
          Cache *writeCache = sst_list[i]->cache();
          writeCache->set_file_name(fileName);
          writeSST(next_line, fileName, sst_list[i]);
          cache_list_[next_line]->push_back(writeCache);
        }
        /* 删除sstlist */
        for (int i = 0; i < write_size; i++) {
          delete sst_list.front();
          sst_list.pop_front();
        }
      }
      /* 下一层容量小于等于要写入的容量 */
      else {
        /* 将下一层所有sstable读入 */
        // for(auto p = cacheList[nextLine] -> begin(); p != cacheList[nextLine] -> end(); p++) {
        //     sstList.push_back(readSST(nextLine, (*p)));
        //     cacheList[nextLine] -> erase(p);
        // }
        while (!cache_list_[next_line]->empty()) {
          Cache *read_in_cache = cache_list_[next_line]->back();
          sst_list.push_back(readSST(next_line, read_in_cache));
          cache_list_[next_line]->pop_back();
        }
        slot[next_line].clear();  //清空slot
        /* sstlist写 2<<nextLine 个到下一行 */
        write_size = 2 << next_line;
        for (int i = 0; i < write_size; i++) {
          Cache *write_cache = sst_list[i]->cache();
          write_cache->set_file_name(i);
          cache_list_[next_line]->push_back(write_cache);
          writeSST(next_line, i, sst_list[i]);
        }
        /* 删除sstlist */
        for (int i = 0; i < write_size; i++) {
          delete sst_list.front();
          sst_list.pop_front();
        }
      }
    }
    next_line++;
  }
  /* 此时要将第0行的cache和sstable清除 */
  // for(auto p = cacheList[0] -> begin(); p != cacheList[0] -> end(); p++) {
  //     deleteSST(0, (*p) -> getFileName());
  //     delete *p;
  //     cacheList[0] -> erase(p);
  // }
  while (!cache_list_[0]->empty()) {
    Cache *delete_cache = cache_list_[0]->back();
    deleteSST(0, delete_cache->file_name());
    delete delete_cache;
    cache_list_[0]->pop_back();
  }
}

bool cmp(std::string &a, std::string &b) {
  int levelA = atoi(a.data() + 6);
  int levelB = atoi(b.data() + 6);
  return levelA < levelB;
}

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir) {
  stoDir = dir;
  time_flag_ = 0;
  /* 从已有sstable中构造初始的cache */
  std::vector<std::string> folder_list;
  int folders_size = utils::scanDir(stoDir, folder_list);
  if (folders_size == 0) {//如果没有folder则要构造第0行
    time_flag_ = 1;
    std::string level0 = stoDir + "/level-0";
    utils::mkdir(level0.data());
    auto *level_zero_cache = new std::list<Cache *>;
    cache_list_.push_back(level_zero_cache);
    slot.emplace_back();
  }
  /* <del>返回的folder是倒着的 level-n level-(n-1) ... level-0</del> */
  /* <del>本机固态上返回的是倒序的，移动硬盘上返回的是顺序的</del> */
  /* <del>所以我检测第一个文件，看是否是0，判断是正序还是倒序</del> */
  /* 甚至是乱序的，因此我先sort再读入 */
  else {
    std::sort(folder_list.begin(), folder_list.end(), cmp);
    for (int i = 0; i < folders_size; i++) {
      auto cache_list = new std::list<Cache *>;
      cache_list_.push_back(cache_list);//如果是顺序，从后往前插
      std::string folder = stoDir + "/" + folder_list[i];
      std::vector<std::string> file_list;
      int files_size = utils::scanDir(folder, file_list);
      bool *slot_flag = new bool[2 << i]();
      int max_file_name = -1;
      for (int j = 0; j < files_size; j++) {
        std::string raw_file_name = folder + "/" + file_list[j];
        Cache *cache = readCache(raw_file_name);
        int file_name = atoi(file_list[j].data());
        slot_flag[file_name] = true;
        max_file_name = file_name > max_file_name ? file_name : max_file_name;
        time_flag_ = cache->getTime() > time_flag_ ? cache->getTime() : time_flag_;
        cache->set_file_name(file_name);
        cache_list->push_back(cache);
      }
      /* 构造slot */
      slot.emplace_back();
      for (int j = 0; j <= max_file_name; j++) {
        if (!slot_flag[j]) slot[i].push_back(j);
      }
      delete[] slot_flag;
    }
    time_flag_++;
  }
}

KVStore::~KVStore() {
  /* 将存在内存中的memtable生成sstable，写入disk */
  SSTable *ss_table = createSSTable();
  if (ss_table) {
    Cache *newCache = ss_table->cache();
    /* begin write sstable */
    if (cache_list_[0]->size() < 2) { //dont need compaction
      int fileName = cache_list_[0]->size();
      newCache->set_file_name(fileName);
      writeSST(0, fileName, ss_table);
      cache_list_[0]->push_back(newCache);
      delete ss_table;
    }
    /* need compaction */
    else {
      compactor(ss_table);
    }
  }
  /* 删除所有cache */
  while (!cache_list_.empty()) {
    for (auto &p : *cache_list_.back()) {
      delete p;
    }
    delete cache_list_.back();
    cache_list_.pop_back();
  }
  mem_table_.reset();
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {
  if (!mem_table_.addEntry(key, s)) {
    SSTable *ss_table = createSSTable();
    Cache *cache = ss_table->cache();
    /* if there is no cacheList[0] (it happens when after reset()) */
    if (cache_list_.empty()) {
      std::string level0 = stoDir + "/level-0";
      utils::mkdir(level0.data());
      auto *list_zero = new std::list<Cache *>; //level0's cache list
      cache_list_.push_back(list_zero);
      slot.emplace_back();
    }
    /* begin write sstable */
    if (cache_list_[0]->size() < 2) {//dont need compaction
      //<del>新的cache直接插到尾部，fileName为cacheList[0]的size</del>
      //新的cache先插slot
      int file_name;
      if (!slot[0].empty()) {
        file_name = slot[0].back();
        slot[0].pop_back();
      } else {
        file_name = cache_list_[0]->size();
      }
      cache->set_file_name(file_name);
      writeSST(0, file_name, ss_table);
      cache_list_[0]->push_back(cache);
      delete ss_table;
    }
    /* need compaction */
    else {
      compactor(ss_table);
    }

    time_flag_++;
    mem_table_.reset();
    mem_table_.addEntry(key, s);
  }
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
  std::string *ret_str_p;
  if ((ret_str_p = mem_table_.search(key)) != nullptr) {  //find in memTable
    if (*ret_str_p == "~DELETE~")
      return "";
    else
      return *ret_str_p;
  } else {  //search in cache if find goto disk to get it
    int file_name = -1;
    uint64_t big_time = 0;
    uint32_t offset = 0, length = 0, final_length = 0;
    uint32_t level = 0;
    for (int i = 0; i < cache_list_.size(); i++) {
      auto p = cache_list_[i]->begin();
      for (; p != cache_list_[i]->end(); p++) { //遍历一层
        if ((*p)->ifExist(key)) { //在cache中存在
          uint64_t tmp_offset = (*p)->binSearch(key, length);  //二分搜索
          if (tmp_offset != 0) {  //二分找到
            uint64_t tmp_time = (*p)->getTime();
            if (tmp_time > big_time) {  //如果时间戳更新
              offset = tmp_offset;
              big_time = tmp_time;
              level = i;
              file_name = (*p)->file_name();
              final_length = length;
            }
          }
        }
      }
    }

    if (file_name == -1) return "";//if not find

    std::string resultDir = getFilePath(level, file_name);
    std::ifstream inFile(resultDir, std::ios::in | std::ios::binary);
    inFile.seekg(offset, std::ios::beg);
    char *ret_str_char = new char[final_length + 1]();
    inFile.read(ret_str_char, final_length);
    std::string ret_str(ret_str_char);
    if (ret_str == "~DELETE~") ret_str.clear();
    delete[] ret_str_char;
    return ret_str;
  }
}
/**
 * GET without cache
 */
#ifdef NOcache
std::string KVStore::get(uint64_t key) {
  std::string *ret_str_p;
  if ((ret_str_p = memTable.search(key)) != nullptr) {//find in memTable
    if (*ret_str_p == "~DELETE~") return "";
    else
      return *ret_str_p;
  } else {
    int fileName = -1;
    uint64_t bigTime = 0;
    uint32_t offset = 0, length = 0, finalLength = 0;
    uint32_t level = 0;
    for (int i = 0; i < cacheList.size(); i++) {
      auto p = cacheList[i]->begin();
      for (; p != cacheList[i]->end(); p++) {//遍历一层
        std::string currentFileName = getFilePath(i, (*p)->getFileName());
        cache *thisCache = readCache(currentFileName);
        if (thisCache->ifExist(key)) {                           //在cache中存在
          uint64_t offsetTmp = thisCache->binSearch(key, length);//二分搜索
          if (offsetTmp != 0) {                                  //二分找到
            uint64_t timeTmp = thisCache->getTime();
            if (timeTmp > bigTime) {//如果时间戳更新
              offset = offsetTmp;
              bigTime = timeTmp;
              level = i;
              fileName = (*p)->getFileName();
              finalLength = length;
            }
          }
        }
        delete thisCache;
      }
    }
    if (fileName == -1) return "";//if not find

    std::string resultDir = getFilePath(level, fileName);
    std::ifstream inFile(resultDir, std::ios::in | std::ios::binary);
    inFile.seekg(offset, std::ios::beg);
    char *ret_str_char = new char[finalLength + 1]();
    inFile.read(ret_str_char, finalLength);
    std::string ret_str(ret_str_char);
    if (ret_str == "~DELETE~") ret_str.clear();
    delete[] ret_str_char;
    return ret_str;
  }
}
#endif
#ifdef PARTcache
/*
 * GET with part cache
 */
std::string KVStore::get(uint64_t key) {
  std::string *ret_str_p;
  if ((ret_str_p = memTable.search(key)) != nullptr) {//find in memTable
    if (*ret_str_p == "~DELETE~") return "";
    else
      return *ret_str_p;
  } else {//search in cache if find goto disk to get it
    int fileName = -1;
    uint64_t bigTime = 0;
    uint32_t offset = 0, length = 0, finalLength = 0;
    uint32_t level = 0;
    for (int i = 0; i < cacheList.size(); i++) {
      auto p = cacheList[i]->begin();
      for (; p != cacheList[i]->end(); p++) {             //遍历一层
                                                          //                if ((*p) -> ifExist(key)) {//在cache中存在
        uint64_t offsetTmp = (*p)->binSearch(key, length);//二分搜索
        if (offsetTmp != 0) {                             //二分找到
          uint64_t timeTmp = (*p)->getTime();
          if (timeTmp > bigTime) {//如果时间戳更新
            offset = offsetTmp;
            bigTime = timeTmp;
            level = i;
            fileName = (*p)->getFileName();
            finalLength = length;
          }
        }
        //                }
      }
    }

    if (fileName == -1) return "";//if not find

    std::string resultDir = getFilePath(level, fileName);
    std::ifstream inFile(resultDir, std::ios::in | std::ios::binary);
    inFile.seekg(offset, std::ios::beg);
    char *ret_str_char = new char[finalLength + 1]();
    inFile.read(ret_str_char, finalLength);
    std::string ret_str(ret_str_char);
    if (ret_str == "~DELETE~") ret_str.clear();
    delete[] ret_str_char;
    return ret_str;
  }
}
#endif
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
  std::string result = get(key);
  if (!result.empty()) {
    put(key, "~DELETE~");
    return true;
  } else
    return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
  /* 删除SSTable和cache */
  for (int i = 0; i < cache_list_.size(); i++) {
    auto list = cache_list_[i];
    while (!list->empty()) {
      Cache *deleteCache = list->back();
      int fileName = deleteCache->file_name();
      deleteSST(i, fileName);
      delete deleteCache;
      list->pop_back();
    }
    if (-1 == utils::rmdir(getFolderPath(i).data())) {
      std::cout << "Cannot delete folder level " << i << std::endl;
    }
  }
  /* 清空cacheList */
  while (!cache_list_.empty()) {
    cache_list_.pop_back();
  }
  /* 清空memTable */
  mem_table_.reset();
  /* 清空slot */
  while (!slot.empty()) {
    slot.pop_back();
  }
  time_flag_ = 1;
}