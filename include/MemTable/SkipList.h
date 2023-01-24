//
// Created by 魏新鹏 on 2021/4/16.
//

#ifndef LSM_LAB_SKIPLIST_H
#define LSM_LAB_SKIPLIST_H

#include "QuadList.h"

#include <list>
//符合Dictionary接口的SkipList模板类（但隐含假设元素之间可比较大小）
class SkipList {
 protected:
  bool skipSearch(std::list<QuadList *>::iterator &qlist, QuadListNode *&p, uint64_t k);

 public:
  bool empty() const { return skip_list_.empty(); }
  uint32_t size() const { return skip_list_.empty() ? 0 : skip_list_.back()->size(); }//底层QuadList的规模
  uint64_t getMax() const { return skip_list_.empty() ? 0 : skip_list_.back()->last()->entry.key; }
  uint64_t getMin() const { return skip_list_.empty() ? 0 : skip_list_.back()->first()->entry.key; }
  void put(uint64_t k, const std::string &v);
  std::string *get(uint64_t k);       //读取
  bool remove(uint64_t k, uint32_t &);//删除
  Entry **getWhole();
  void clear();

 private:
  std::list<QuadList *> skip_list_;//一条竖着的双链表
};

#endif//LSM_LAB_SKIPLIST_H