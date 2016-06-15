/*!
 * \file compute_communicator.h
 * \brief 计算节点的网络模块
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef __DCR__compute_communicator__
#define __DCR__compute_communicator__

#include "../network/communicator.h"
#include "../util/mutex.h"
#include "../util/condition_variable.h"
#include "../util/logger.h"
#include "../util/common.h"
#include "../network/tcp_socket.h"
#include "../network/udp_socket.h"
#include "../network/network_packet.h"
#include "../dcr/caculator.h"
#include "../util/flags.h"
#include "../core_data_structure/memory_queue.h"
#include "../dcr/dcr_base_class.h"

#include <pthread.h>
#include <unistd.h>
#include <signal.h>

///
/// \brief 计算节点的网络通信模块
///
class ComputeCommunicator : public Communicator {
public:
    ComputeCommunicator() {}
    
    ~ComputeCommunicator() {
        this->destroy();
    }
    
    /// \brief 开启各项服务线程
    ///
    /// 1.开启任务接受线程
    /// 2.连接调度节点
    /// 3.如果连接成功则开启心跳发送线程
    /// 4.开启结束命令监听线程
    bool startWork();
    
    ///
    /// \brief 结束各项服务线程
    ///
    /// 1.等待结束命令监听线程返回
    /// 1.结束任务接受线程
    /// 3.结束心跳发送
    bool endWork();
    
    ///
    /// \brief 初始化任务队列, 获取调度节点IP, ACK报文
    ///
    bool init();
    
    /// \brief 计算发送结果
    /// \param compute_result 要发送计算结果
    /// \param udp_task 计算结果对应
    bool sendResult(Result *compute_result, UdpSendTask *udp_task);
    
    ///
    /// \brief 根据原始任务ID获取原始任务，如果在map中则直接取出，否则向调度节点请求
    ///
    OriginalTask* getOriginalTask(int32 original_task_ID);
    
    ///
    /// \brief 向调度节点发送登陆信息，等待ACK的返回
    ///
    bool connectToScheduler();

    FunctionalQueue* getTaskQueue();

    ///
    /// \brief 获取调度节点IP
    ///
    void getSchedulerIP(char *scheduler_ip);
private:
    ///
    /// \brief 销毁init中创建的对象
    ///
    bool destroy();
    
    ///
    /// \brief 接受任务放入任务队列, 线程函数, 如果队列长度已经满了，则循环等待
    ///
    static void *receiveTask(void *arg);
    
    ///
    /// \brief 发送心跳包, 线程函数
    ///
    static void *heartBeat(void *arg);
    
    ///
    /// \brief 接受结束命令, 线程函数
    ///
    static void *receiveEndWork(void *arg);
    
    char* m_local_ip;                           ///< 计算节点本地IP
    char* m_scheduler_ip;                       ///< 调度节点IP
    Mutex* m_mutex;                             ///< 互斥锁
    map<int32, OriginalTask*> m_original_task;  ///< 保存原始任务
    LoginAck* m_reserve_info;                   ///< 计算几点保存节点在调度节点中的信息
    FunctionalQueue* m_task_queue;              ///< 用于计算的任务队列
    pthread_t m_send_heart_beat;                ///< 心跳线程的ID
    pthread_t m_receive_task;                   ///< 任务接受线程的ID
    pthread_t m_end_work;                       ///< 结束包侦听线程的ID
    
    friend class Caculator;
};

#endif 
