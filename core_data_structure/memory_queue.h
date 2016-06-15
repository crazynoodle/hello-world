/*!
 * \file memory_queue.h
 * \brief 内存池
 *
 * 主要有4种内存池,任务内存池
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef DCR_memory_queue_h
#define DCR_memory_queue_h

#include "../util/common.h"
#include "../util/mutex.h"
#include "../core_data_structure/functional_queue.h"
#include "../util/logger.h"

class FunctionalQueue;

///
/// \brief 内存池的类型
///
enum memory_queue_type {
    MQ_TASK,                ///< 任务内存池(UdpTask)
    MQ_LOGIN,               ///< 登陆包内存池
    MQ_RESULT,              ///< 计算结果内存池(Udp)
    MQ_COMPUTE_NODE         ///< 计算节点内存池
};

typedef memory_queue_type memory_queue_t;

///
/// \brief 用于将内存池组织成链表的结构, 每个内存单元的索引,
///        保存了前链表元素的内存索引和后面链表元素的内存索引
///
typedef struct UnitIndex {
    int32 prev;                             ///< 前面一个内存单元的索引
    int32 next;                             ///< 后面一个内存单元的索引
    FunctionalQueue* fun_queue_pointer;     ///< 指向的FunctionalQueue
    time_t time;                            ///< 用与node中的两个队列保存加载时间,便于判断是否超时
} unit_index_t;

///
/// \brief 内存池
///
class MemoryQueue {
public:
    MemoryQueue() {}
    
    ~MemoryQueue() {
        this->destroyRealQueue();
    }
    /// \brief 初始化
    ///
    /// 1.分配内存
    /// 2.初始化m_global_free
    /// 3.初始化m_unit_index
    /// \param mq_type 内存池类型
    /// \param unit_size 内存单元的大小
    /// \param queue_len 内存单元的个数
    /// \return true为存在， false为不存在
    bool init(memory_queue_t mq_type, int32 unit_size, int32 queue_len);
    
    /// \brief 通过索引获取一个内存单元
    /// \param 获取内存单元的索引
    /// \return 返回的内存单元
    void* getOneBodyByIndex(int32 index);
    
    /// \brief 通过索引获取一个内存单元索引
    /// \param 获取内存单元索引的索引
    /// \return 返回的内存单元索引
    unit_index_t* getOneUnitIndexByIndex(int32 index);
    
    void setFunQueueListHead(FunctionalQueue* head);
    
    void* getBody();
    
    unit_index_t* getUnitIndex();
    
    int32 getQueueLen();
    
    FunctionalQueue* getFunQueueListHead();
    
    FunctionalQueue* getGlobalFree();

private:
    ///
    /// \brief 销毁init中创建的对象
    ///
    void destroyRealQueue();
    
    memory_queue_t m_type;                  ///< 内存池的类型
    void *m_queue;                          ///< 指向内存池的指针
    unit_index_t *m_unit_index;             ///< 指向内存池索引的指针
    int32 m_unit_size;                      ///< 每个内存单元的大小
    int32 m_queue_len;                      ///< 内存池有多少个内存单元
    FunctionalQueue *m_global_free;         ///< 全局global_free队列
    FunctionalQueue *m_fun_queue_list_head; ///< 将属于这个内存池的FunctionalQueue组织成一个
    mutable Mutex *m_mutex;                 ///< 互斥锁
};
#endif
