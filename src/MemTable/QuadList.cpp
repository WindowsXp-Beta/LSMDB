//
// Created by 魏新鹏 on 2021/4/17.
//

#include "QuadList.h"

void QuadList::init() {       //QuadList初始化，创建QuadList对象时统一调用
  header_ = new QuadListNode; //创建头哨兵节点
  trailer_ = new QuadListNode;  //创建尾哨兵节点
  header_->succ = trailer_;
  header_->pred = nullptr;  //沿横向联接哨兵
  trailer_->pred = header_;
  trailer_->succ = nullptr;                  //沿横向联接哨兵
  header_->above = trailer_->above = nullptr;//纵向的后继置空
  header_->below = trailer_->below = nullptr;//纵向的前驱置空
  size_ = 0;                                 //记录规模
} //如此构造的四联表，不含任何实质的节点，且暂时与其它四联表相互独立

int QuadList::clear() {
  int oldSize = size_;
  while (0 < size_) remove(header_->succ);//逐个删除所有节点
  return oldSize;
}

Entry QuadList::remove(QListNodePosi p) {
  p->pred->succ = p->succ;
  p->succ->pred = p->pred;
  size_--;
  Entry p_val = p->entry;
  delete p;
  return p_val;
}

QListNodePosi QuadList::insertAfterAbove(const Entry &e, QListNodePosi p, QListNodePosi b) {
  size_++;
  return p->insertAsSuccAbove(e, b);
}

Entry **QuadList::getWhole() {
  Entry **content = new Entry *[size_];
  QListNodePosi p = header_->succ;
  for (int i = 0; p != trailer_; i++, p = p->succ) {
    content[i] = &(p->entry);
  }
  return content;
}