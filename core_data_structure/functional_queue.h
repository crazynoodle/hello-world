/*!
 * \file funtional_queue.h
 * \brief 用于将内存池组织成多个链表的数据结构
 *
 * funtional_queue的使用模式主要有：
 * 1.创建temp队列, 初始化, 从global中分配，赋值后，再把temp队列中的放到其他队列中
 * 2.创建temp队列, 初始化, 从其他队列中moveToTemp, 再用另一个队列moveFromTemp
 * 3.两个队列直接进行moveOneElementTo不过要加锁
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef __DCR__functional_queue__
#define __DCR__functional_queue__

#include "../util/common.h"
#include "../util/mutex.h"
#include "../util/logger.h"

class MemoryQueue;
struct UnitIndex;
typedef struct UnitIndex unit_index_t;

///
/// \brief FunctionalQueue的类型
///
enum functional_queue_type {
    FQ_GLOBAL_FREE,
    FQ_DECOMPOSER,
    FQ_DECOMPOSER_TEMP,
    FQ_TO_SEND,
    FQ_TO_DELETE,
    FQ_GROUP_FREE,
    FQ_LOGIN,
    FQ_LOGIN_TEMP,
    FQ_ACTIVE_CN,
    FQ_ACTIVE_CN_TEMP,
    FQ_RESULT,
    FQ_RESULT_TEMP,
    FQ_TASK,
    FQ_TASK_TEMP,
    FQ_TCP_RECV,
    FQ_TCP_RECV_TEMP,
    FQ_UDP_RECV,
    FQ_UDP_RECV_TEMP
};

typedef enum functional_queue_type fun_queue_t;

///
/// \brief 用于将内存池组织成多个链表的数据结构
///
class FunctionalQueue {
public:
    FunctionalQueue() {}
    
    ~FunctionalQueue() {
        this->destroy();
    }
    
    /// \brief 初始化函数
    /// \param memory_queue 对应的内存池
    /// \param type FunctionalQueue的类型
    /// \param group_seq FunctionalQueue的组号, 如果不属于分组则默认为0
    /// \param len 队列长度，默认初始化为0，global_free是个例外
    /// \param head FunctionQueue 在对应的内存池中的开始索引
    /// \param tail FunctionQueue 在对应的内存池中的结束索引
    /// \return 初始化成功返回true, 失败返回false
    bool init(MemoryQueue *memory_queue, fun_queue_t type, int32 group_seq = 0,
              int32 len = 0, int32 head = -1, int tail = -1);
    
    /// \brief 将一个队列中的内容移动到一个temp队列中
    ///
    /// 1.检查参数的正确性
    /// 2.加锁，比较src的长度与move_len的长度
    /// 3.如果src的长度比move_len长, 移动move_len的长度，否则移动src的长度
    ///   从链表头开始截取
    /// 4.解锁
    /// 5.改变已经截取的内存池的队列指向
    /// 6.将截取的长度合并入目标队列，并入目标队列的头部
    /// \param dest 目标队列
    /// \param move_len 要移动的长度
    /// \return 实际移动的长度
    int32 moveToTemp(FunctionalQueue *dest, int32 move_len);
    
    /// \brief 将temp队列中的内容移动到一个队列中
    ///
    /// 1.检查参数的正确性
    /// 2.比较src的长度与move_len的长度
    /// 3.如果src的长度比move_len长, 移动move_len的长度，否则移动src的长度
    ///   从链表头开始截取
    /// 4.改变已经截取的内存池的队列指向
    /// 5.加锁
    /// 6.将截取的长度合并入目标队列
    /// 7.解锁
    /// \param dest 目标队列
    /// \param move_len 要移动的长度
    /// \return 实际移动的长度
    int32 moveFromTemp(FunctionalQueue *src, int32 move_len);
    
    /// \brief 将一个队列中的内容移动到另一个队列中(无加锁, 所以外部要加锁)
    ///
    /// 1.检查参数的正确性
    /// 2.把index对应的内存单元移出src队列,分为头，中间，尾三种情况
    /// 3.改变内存单元的队列指向
    /// 4.将内存单元并入dest队列
    /// \param dest 目标队列
    /// \param move_len 要移动的内存单元的索引
    /// \return 实际移动的长度
    int32 moveOneElementTo(FunctionalQueue *dest, int32 move_index);
    
    /// \brief 从globalFree中给temp队列分配空间
    ///
    /// 1.调用moveToTemp
    /// \param len 要移动的长度
    /// \return 分配的长度
    int32 tempAllocFromGlobalFree(int32 len);
    
    /// \brief 将临时队列的内容放回globalFree
    /// \return 释放的长度
    int32 tempReleaseToGlobalFree();
    
    /// \brief 检查index对应的内存池中的单元是不是在此functionalQueue中
    /// \param index 索引
    /// \return true为存在， false为不存在
    bool checkIndex(int32 index);
    
    /// \brief 通过索引获取一个memorybody
    /// \param index 索引
    /// \return 获取的memorybody
    void* getMemoryBody(int32 index);
    
    /// \brief 通过索引获取一个unitIndex
    /// \param index 索引
    /// \return 获取的unitIndex
    unit_index_t* getOneUnitIndex(int32 index);
    
    /// \brief 当前FunctionalQueue索引的下一个元素的在内存池中索引
    /// \param fq_index 当前索引
    /// \return 当前FunctionalQueue索引的下一个元素的索引
    int32 getFqNextByIndex(int32 fq_index);
    
    /// \brief 当前FunctionalQueue索引的前一个元素的在内存池中索引
    /// \param fq_index 当前索引
    /// \return 当前FunctionalQueue索引的前一个元素的索引
    int32 getFqPrevByIndex(int32 fq_index);
    
    /// \brief 获取下一条虚拟队列
    FunctionalQueue* getNextFq();
    
    ///
    /// \brief 锁住FunctionalQueue
    ///
    void lockFq();
    
    ///
    /// \brief 解锁FunctionalQueue
    ///
    void unlockFq();
    
    MemoryQueue* getMemoryQueue();
    
    int32 getGroupSeq();
    
    int32 getLen();
    
    int32 getHead();
    
    int32 getTail();
    
private:
    ///
    /// \brief 销毁init中创建的对象
    ///
    void destroy();
    
    fun_queue_t m_type;                         ///< functional_queue的类型
    int32 m_group_seq;                          ///< 对应的分组的组号
    int32 m_len;                                ///< 队列的长度
    int32 m_head;                               ///< 队列的开始索引
    int32 m_tail;                               ///< 队列的结束索引
    MemoryQueue *m_memory_queue;                ///< 队列对应的内存池
    FunctionalQueue *m_next_fun_queue;          ///< 内存池上的FunctionalQueue也组织成链表
    mutable Mutex *m_mutex;                     ///< 互斥锁
};

#endif
