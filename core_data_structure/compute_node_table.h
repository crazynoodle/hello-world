/*!
 * \file compute_node_table.h
 * \brief 调度节点维护计算节点相关信息
 *
 * 文件包含了ComputeNodeInfo, GroupInfo, GroupInTable, ComputeNodeTable
 * 四个类， 将ComputeNodeInfo 通过一个表ComputeNodeTable组织起来， 每个表由N
 * 个组GroupInTable组成，每个组还有一个GroupInfo用于保存每个组的相关信息
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */


#ifndef __DCR__compute_node_table__
#define __DCR__compute_node_table__

#include "../util/common.h"
#include "../core_data_structure/memory_queue.h"
#include "../core_data_structure/functional_queue.h"
#include "../network/network_packet.h"
#include "../dcr/dcr_base_class.h"


#include <unordered_set>
#include <string>

class Strategy;
class StrategyBlast;
///
/// \brief 调度节点维护的计算节点的信息
///
class ComputeNodeInfo {
public:
    ComputeNodeInfo() {}
    
    ~ComputeNodeInfo() {
        this->destroy();
    }
    
    /// \brief 初始化函数
    /// \param ip 所对应计算节点的IP
    /// \param last_access 计算节点最新一次和调度节点联系的时间
    /// \param task_len 计算节点目前的任务包的个数
    /// \return 初始化成功返回true, 失败返回false
    bool init(char* ip, time_t last_access, int32 task_len);
    
private:
    ///
    /// \brief 用于销毁init中创建的对象
    ///
    void destroy();
    
    FunctionalQueue m_task_wait_to_send;     ///< 将要发送给计算节点的队列
    FunctionalQueue m_task_already_send;     ///< 已经发送给计算节点的队列
    Mutex m_mutex;                           ///< 互斥锁
    int32 m_cq_task_len;                     ///< 计算节点现在的任务长度
    time_t m_last_access;                    ///< 计算节点最新一次和调度节点联系的时间
    char ip[16];                             ///< 计算节点的IP
    //map<int32, int32> m_type_prior;
    //unordered_set<int32> s_type;           
    friend class GroupInTable;
    friend class Dispatcher;
    friend class Strategy;
    friend class StrategyBlast;
    friend class StrategyType;
};


///
/// \brief 计算节点表每个分组的相关信息
///
class GroupInfo {
public:
    GroupInfo() {}
    
    ~GroupInfo() {
        this->destroy();
    }
    
    ///
    /// \brief 初始化函数
    ///
    bool init();
    
    ///
    /// \brief 销毁init中创建的对象
    ///
    void destroy();
    
public:
    int32 m_load_task_num_all;             ///< 所有调度周期从分解队列加载的任务数
    int32 m_send_task_num_all;             ///< 所有调度周期从m_wait_queue发送给计算节点的任务数
    int32 m_load_task_num;                 ///< 一个调度周期加载的任务数
    int32 m_send_task_num;                 ///< 一个调度周期发送的任务数
    int32 m_redispatch_num_one_cycle;      ///< 每个调度周期检查得到的超时需要重新发送的任务数
    int32 m_redispatch_num_all_cycles;     ///< 所有调度周期需要重新发送的任务的数量
    int32 m_new_add_node_num;              ///< 每个任务周期新增加的计算节点的数量
    int32 m_disconnect_node_num;           ///< 所有周期挂掉的计算节点的数量
    int32 m_disconnect_node_per_cycle;     ///< 一个周期挂掉的节点的数量
    int32 m_active_node_num;               ///< 该分组现在还有的活跃节点的数量
    int32 m_processed_result_num;          ///< 每个调度周期处理的计算结果的数量
};


///
/// \brief 计算节点分组
///
class GroupInTable {
public:
    GroupInTable() {}
    
    ~GroupInTable() {
        this->destroy();
    }
    
    /// \brief 初始化函数
    ///
    /// 初始化过程主要是:创建每个分组并调用其初始化函数进行初始化.
    /// 1.创建4个FunctionalQueue(login,result, active_node, group_free),
    /// 2.一个MemoryQueue(计算节点队列)
    /// 3.分组信息, 互斥锁
    /// \param num_per_group 每个分组有多少个计算节点
    /// \param group_seq 每个分组在计算节点表中的组号
    /// \return 初始化成功返回true, 失败返回false
    bool init(int32 num_per_group, int32 group_seq);
    
    uint32 getComputeNodeLen();
    
    MemoryQueue *getComputeNodeQueue();
    
    GroupInfo *getInfo();
    
    FunctionalQueue *getResult();
    
    FunctionalQueue *getLogin();
    
    FunctionalQueue *getActiveComputeNode();
    
    FunctionalQueue *getGroupFree();
    
private:
    ///
    /// \brief 销毁init中创建的对象
    ///
    void destroy();

    int32 m_group_seq;                              ///< 分组的组号
    MemoryQueue *m_compute_node_queue;              ///< 计算节点的实体队列
    uint32 m_compute_node_len;                      ///< 实体队列的长度
    GroupInfo *m_info;                              ///< 分组的信息
    FunctionalQueue *m_result;                      ///< 分组的结果队列
    FunctionalQueue *m_login;                       ///< 分组的登陆队列
    FunctionalQueue *m_active_compute_node;         ///< 分组的活跃节点队列
    FunctionalQueue *m_group_free;                  ///< 分组的空闲任务队列
    
    friend class Strategy;
    friend class StrategyBlast;
    friend class StrategyType;
};

///
/// \brief 计算节点表
///
class ComputeNodeTable {
public:
    ComputeNodeTable() {}
    
    ~ComputeNodeTable() {
        this->destroy();
    }
    
    /// \brief 初始化函数
    ///
    /// 初始化过程主要是创建每个分组并调用其初始化函数进行初始化
    /// \param group_num 计算节点表有多少个分组
    /// \param num_per_group 每个分组有多少个计算节点
    /// \return 初始化成功返回true, 失败返回false
    bool init(int32 group_num, int32 num_per_group);
    
    int32 getGroupNum();
    
    GroupInTable *getGroup(int32 group_index);
    
private:
    ///
    /// \brief 销毁init中创建的对象
    ///
    void destroy();
    
    int32 m_group_num;                              ///< 分组数量
    GroupInTable *m_group;                          ///< 计算节点表中的分组的指针
    unordered_set<unsigned long> active_ip_list;    ///< 所有活跃节点的IP， 用于返回结果时检查节点的IP是否还在活跃节点表中
    
    friend class GroupInTable;
    friend class Strategy;
    friend class StrategyBlast;
    friend class StrategyType;
    friend class Decomposer;
};

#endif 
