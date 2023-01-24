//
// Created by 魏新鹏 on 2021/4/16.
//

#ifndef LSM_LAB_QUADLISTNODE_H
#define LSM_LAB_QUADLISTNODE_H

#include "Entry.h"
struct QuadListNode;
using QListNodePosi = QuadListNode *;//跳转表节点位置
struct QuadListNode {
  Entry entry;//所存词条
  QListNodePosi pred;
  QListNodePosi succ;//前驱、后继
  QListNodePosi above;
  QListNodePosi below; //上邻、下邻
  explicit QuadListNode//构造器
      (Entry e = Entry(), QListNodePosi p = nullptr, QListNodePosi s = nullptr,
       QListNodePosi a = nullptr, QListNodePosi b = nullptr)
      : entry(e), pred(p), succ(s), above(a), below(b) {}
  QListNodePosi insertAsSuccAbove//插入新节点，以当前节点为前驱，以节点b为下邻
      (Entry const &e, QListNodePosi b = nullptr) {
    QListNodePosi x = new QuadListNode(e, this, succ, nullptr, b);//创建新节点
    succ->pred = x;
    succ = x;           //设置水平逆向链接
    if (b) b->above = x;//设置垂直逆向链接
    return x;           //返回新节点的位置
  }
};

#endif//LSM_LAB_QUADLISTNODE_H
