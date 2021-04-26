//
// Created by 魏新鹏 on 2021/4/16.
//

#include "SkipList.h"

bool SkipList::skipSearch ( std::list<QuadList*>::iterator &qlist, //从指定层qlist的
                            QuadListNode* &p, //首节点p出发 注意首节点和头结点的差别 首节点是 header->succ
                            uint64_t k ) { //向右、向下查找目标关键码k
    while ( true ) { //在每一层
        while ( p -> succ && ( p -> entry.key <= k ) ) //从前向后查找 p->succ 防止是尾结点 尾结点的的entry使用 T()初始化。
            p = p -> succ; //直到出现更大的key或溢出至trailer
        p = p -> pred; //此时倒回一步，即可判断是否
        if ( p -> pred && ( k == p -> entry.key ) ) return true; //命中
        qlist++; //否则转入下一层
        if ( qlist == SkList.end() ) return false; //若已到穿透底层，则意味着失败
        p = ( p->pred ) ? p->below : (*qlist) -> first(); //否则转至当前塔的下一节点 若p是头节点 则p是没有below的 在Qlist的构造函数中，将Qlist的头尾节点的above和below全部置为null
    }  //课后：通过实验统计，验证关于平均查找长度的结论
}

void SkipList::put ( uint64_t k, const std::string &v ) { //跳转表词条插入算法
    Entry e = Entry( k, v ); //待插入的词条（将被随机地插入多个副本）
    if ( empty() ) {
        QuadList * new_quadlist = new QuadList;
        SkList.push_back(new_quadlist);
    } //插入首个Entry

    std::list<QuadList*>::iterator qlist = SkList.begin(); //从顶层四联表的
    QListNodePosi p = (*qlist) -> first(); //首节点出发
    if ( skipSearch ( qlist, p, k ) ) //查找适当的插入位置（不大于关键码k的最后一个节点p）
    {
        //若已有雷同词条，进行替换
        do {
            p -> entry.value = v;
        } while ( p = p -> below );
    }
    else {
        qlist = SkList.end();
        qlist--; //以下，将从p的右侧，产生一座新塔

        QListNodePosi b = (*qlist)->insertAfterAbove(e, p); //新节点b即新塔基座
        while (rand() & 1) { //经投掷硬币，若确定新塔需要再长高一层，则
            while ((*qlist)->valid(p) && !p->above) p = p->pred; //找出不低于此高度的最近前驱
            if (!(*qlist)->valid(p)) { //若该前驱是header
                if (qlist == SkList.begin()) //且当前已是最顶层，则意味着必须
                {
                    QuadList *new_quadlist = new QuadList;
                    SkList.push_front(new_quadlist); //首先创建新的一层，然后
                }
                p = (*(--qlist))->first()->pred; //将p转至上一层Skiplist的header，因为各header之间是不连接的
            } else {//否则，可径自
                p = p->above; //将p提升至该高度
                qlist--;
            }
            b = (*qlist)->insertAfterAbove(e, p, b); //将新节点插入p之后、b之上
        }
    }
}

std::string * SkipList::get(uint64_t k) {
    if ( SkList.empty() ) return nullptr;
    std::list<QuadList*>::iterator qlist = SkList.begin();//从顶层
    QListNodePosi p = (*qlist) -> first();//首节点出发
    if (skipSearch(qlist, p, k)) return &(p -> entry.value);
    else return nullptr;
}

bool SkipList::remove(uint64_t k, uint32_t &length) {
    if ( SkList.empty() ) return false;//根据cppreference，当链表为空时，引用begin，end之类的东西是不行的
    std::list<QuadList*>::iterator qlist = SkList.begin();//从顶层
    QListNodePosi p = (*qlist) -> first();//的首节点
    if (skipSearch(qlist, p, k)) {//找到该节点，删除这一整座塔
        length = p -> entry.value.length();
        do {
            QListNodePosi next_p = p -> below;
            (*qlist) -> remove(p);
            qlist++;
            p = next_p;
        } while (qlist != SkList.end());
        while (!SkList.empty() && SkList.front() -> empty()) {//删除空了的顶层，同理，当你使用front时，要先检查链表是不是空了
            SkList.pop_front();
        }
        return true;
    }
    /* if not find */
    length = 0;
    return false;
}

void SkipList::clear() {
    while (!SkList.empty()) {
        delete SkList.front();
        SkList.pop_front();
    }
}

Entry **SkipList::getWhole() {
    return SkList.back() -> getWhole();
}
