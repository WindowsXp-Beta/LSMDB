//
// Created by 魏新鹏 on 2021/4/16.
//

#ifndef LSM_LAB_QUADLIST_H
#define LSM_LAB_QUADLIST_H

#include "QuadListNode.h"

class QuadList {
private:
    int _size;
    QListNodePosi header, trailer; //规模、头哨兵、尾哨兵
protected:
    void init(); //QuadList创建时的初始化
    int clear(); //清除所有节点
public:
// 构造函数
    QuadList() { init(); } //默认
// 析构函数
    ~QuadList() { clear(); delete header; delete trailer; } //删除所有节点，释放哨兵
// 只读访问接口
    int size() const { return _size; } //规模
    bool empty() const { return _size <= 0; } //判空
    QListNodePosi first() const { return header->succ; } //首节点位置
    QListNodePosi last() const { return trailer->pred; } //末节点位置
    bool valid ( QListNodePosi p ) //判断位置p是否对外合法
    { return p && ( trailer != p ) && ( header != p ); }
// 可写访问接口
    Entry remove ( QListNodePosi p ); //删除（合法）位置p处的节点，返回被删除节点的数值
    QListNodePosi insertAfterAbove ( Entry const& e, QListNodePosi p, QListNodePosi b = nullptr ); //将*e作为p的后继、b的上邻插入
// 遍历
    void traverse ( void (* ) ( Entry& ) ); //遍历各节点，依次实施指定操作（函数指针，只读或局部修改）
//    template <typename VST> //操作器
//    void traverse ( VST& ); //遍历各节点，依次实施指定操作（函数对象，可全局性修改节点）
}; //QuadList

#endif //LSM_LAB_QUADLIST_H
