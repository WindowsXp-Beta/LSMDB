//
// Created by 魏新鹏 on 2021/4/16.
//

#ifndef LSM_LAB_SKIPLIST_H
#define LSM_LAB_SKIPLIST_H

#include <list>
#include "QuadList.h"
//符合Dictionary接口的SkipList模板类（但隐含假设元素之间可比较大小）
class SkipList {
protected:
    bool skipSearch ( std::list<QuadList*>::iterator &qlist, QuadListNode* &p, uint64_t k );
public:
    bool empty() const { return SkList.empty(); }
    int size() const { return SkList.empty() ? 0 : SkList.back() -> size(); } //底层QuadList的规模
    int level() { return SkList.size(); } //层高 == #QuadList，不一定要开放
    bool put ( uint64_t k, const std::string &v ); //插入（注意与Map有别——SkipList允许词条重复，故必然成功）
    std::string * get ( uint64_t k ); //读取
    bool remove ( uint64_t k ); //删除
    void clear();
private:
    std::list<QuadList*> SkList; //一条竖着的双链表
};

#endif //LSM_LAB_SKIPLIST_H
