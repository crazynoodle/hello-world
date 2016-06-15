/*!
 * \file system_init.h
 * \brief 负责系统初始化
 *
 * 主要包括了计算阶段和调度节点上的初始化， 线程启动和全局资源分配函数
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef __DCR__system_init__
#define __DCR__system_init__
#include "../util/scoped_ptr.h"
#include "../util/logger.h"
#include "../util/flags.h"
#include "../core_data_structure/memory_queue.h"
#include "../core_data_structure/functional_queue.h"
#include "../core_data_structure/compute_node_table.h"
#include "../network/network_packet.h"
#include "../network/scheduler_communicator.h"
#include "../network/compute_communicator.h"
#include "../dcr/dispacher.h"
#include "../dcr/caculator.h"
 #include "../dcr/task_manager.h"
#include "../util/config.h"

///
/// \brief 主要包括一些全局的初始化函数和资源分配函数，另外还有和gflag相关的函数
///
namespace dcr {
    //计算节点队列
    scoped_ptr<MemoryQueue>& g_getComputationTaskQueue();
    //登录队列
    scoped_ptr<MemoryQueue>& g_getLoginQueue();
    //结果队列
    scoped_ptr<MemoryQueue>& g_getResultQueue();
    //计算节点表
    scoped_ptr<ComputeNodeTable>& g_getComputeNodeTable();
    //分解器，用于给用户描述计算如何分解原始任务
    scoped_ptr<Decomposer>& g_getDecomposer();
    //调度节点与计算节点通信的模块
    scoped_ptr<SchedulerCommunicator>& g_getSchedulerCommunicator();
    //任务分配器，从分解器加载计算任务, 并分发到计算节点
    scoped_ptr<Dispatcher>& g_getDispatcher();
    
    scoped_ptr<Job>& g_getJob();
    //策略，分解器，dispatcher等各种策略
    scoped_ptr<Strategy>& g_getStrategy();
    //系统配置
    scoped_ptr<SystemConfig>& g_getConfig();

    scoped_ptr<TaskManager>& g_getTaskManager();

    scoped_ptr<XMLLog>& g_getXMLLog();

    scoped_ptr<Asynchronous>& g_getAsynchronous();
    
    /// \brief 调度节点初始化
    ///
    /// 1. 验证参数的正确性
    /// 2. 初始化日记系统
    /// 3. 初始化创建各种内存池(任务， 结果， 登陆，计算节点表)
    /// 4. 初始化分解器
    bool g_SchedulerNodeInitialize();
    
    /// \brief 调度节点任务开启
    ///
    /// 1. 网络模块任务开启
    /// 2. 分解模块任务开启
    /// 3. 分发模块任务开启
    bool g_SchedulerWork();
    
    /// \brief 调度节点任务结束
    ///
    /// 1. 等待各分发线程结束
    /// 2. 结束分解模块
    /// 3. 结束网络模块
    /// 4. 输出相关的信息
    bool g_SchedulerFinilize();
    //计算节点的任务队列
    scoped_ptr<MemoryQueue>& g_getTaskQueueOfComputeNode();
    //计算节点的通信模块
    scoped_ptr<ComputeCommunicator>& g_getComputeCommunicator();
    //计算器
    scoped_ptr<Caculator>& g_getCaculator();
    
    /// \brief 计算节点初始化
    ///
    /// 1. 验证参数的正确性
    /// 2. 初始化日记系统
    /// 3. 初始化创建各种任务内存池
    /// 4. 初始化网络接收器
    bool g_ComputeNodeInitialize();
    
    /// \brief 计算节点任务开启
    ///
    /// 1. 网络模块任务开启
    /// 2. 计算模块任务开启
    bool g_ComputeWork();
    
    /// \brief
    ///
    /// 1. 等待计算模块结束
    /// 2. 结束网络模块
    /// 3. 输出相关信息
    bool g_ComputeFinilize();
}

#endif
