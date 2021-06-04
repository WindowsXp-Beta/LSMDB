//
// Created by 魏新鹏 on 2021/4/17.
//

#include "QuadList.h"

void QuadList::init() { //QuadList初始化，创建QuadList对象时统一调用
    header = new QuadListNode; //创建头哨兵节点
    trailer = new QuadListNode; //创建尾哨兵节点
    header -> succ = trailer; header -> pred = nullptr; //沿横向联接哨兵
    trailer -> pred = header; trailer -> succ = nullptr; //沿横向联接哨兵
    header -> above = trailer -> above = nullptr; //纵向的后继置空
    header -> below = trailer -> below = nullptr; //纵向的前驱置空
    _size = 0; //记录规模
} //如此构造的四联表，不含任何实质的节点，且暂时与其它四联表相互独立

int QuadList::clear() {
    int oldSize = _size;
    while ( 0 < _size ) remove ( header -> succ ); //逐个删除所有节点
    return oldSize;
}

Entry QuadList::remove ( QListNodePosi p ) {
    p -> pred -> succ = p -> succ;
    p -> succ -> pred = p -> pred;
    _size--;
    Entry p_val = p -> entry;
    delete p;
    return p_val;
}

QListNodePosi QuadList::insertAfterAbove(const Entry &e, QListNodePosi p, QListNodePosi b) {
    _size++;
    return p -> insertAsSuccAbove(e, b);
}

void QuadList::traverse ( void ( *visit ) ( Entry& ) ) { //利用函数指针机制，只读或局部性修改
    QListNodePosi p = header;
    while ( ( p = p->succ ) != trailer ) visit ( p -> entry );
}

Entry ** QuadList::getWhole() {
    Entry ** content = new Entry*[_size];
    QListNodePosi p = header -> succ;
    for (int i = 0; p != trailer; i++, p = p -> succ) {
        content[i] = &(p -> entry);
    }
    return content;
}