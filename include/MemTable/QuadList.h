//
// Created by 魏新鹏 on 2021/4/16.
//

#ifndef LSM_LAB_QUADLIST_H
#define LSM_LAB_QUADLIST_H

#include "QuadListNode.h"

class QuadList {
 private:
  int size_;
  QListNodePosi header_, trailer_;//规模、头哨兵、尾哨兵
 protected:
  void init();//QuadList创建时的初始化
  int clear();//清除所有节点
 public:
  // 构造函数
  QuadList() { init(); }//默认

  ~QuadList() {// 析构函数
    clear();  //删除所有节点，释放哨兵
    delete header_;
    delete trailer_;
  }
                                                      // 只读访问接口
  int size() const { return size_; }                  //规模
  bool empty() const { return size_ <= 0; }           //判空
  QListNodePosi first() const { return header_->succ; }//首节点位置
  QListNodePosi last() const { return trailer_->pred; }//末节点位置
  bool valid(QListNodePosi p)                         //判断位置p是否对外合法
  { return p && (trailer_ != p) && (header_ != p); }
  // 可写访问接口
  Entry remove(QListNodePosi p);                                                             //删除（合法）位置p处的节点，返回被删除节点的数值
  QListNodePosi insertAfterAbove(Entry const &e, QListNodePosi p, QListNodePosi b = nullptr);//将*e作为p的后继、b的上邻插入
  Entry **getWhole();
};//QuadList

#endif//LSM_LAB_QUADLIST_H