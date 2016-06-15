/*!
 * \file scheduler_communicator.h
 * \brief 调度节点的网络模块
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef DCR_scheduler_communicator_h
#define DCR_scheduler_communicator_h

#include "../network/communicator.h"
#include "../network/tcp_socket.h"
#include "../network/udp_socket.h"
#include "../util/common.h"
#include "../network/network_packet.h"
#include "../core_data_structure/compute_node_table.h"
 #include "../dcr/task_manager.h"

#include <pthread.h>
#include <unistd.h>
#include <signal.h>

///
/// \brief 调度节点与计算节点通信的模块
///
class SchedulerCommunicator : public Communicator {
public:
    SchedulerCommunicator() {}
    
    ~SchedulerCommunicator() {
        this->destroy();
    }
    
    ///
    /// \brief 初始化函数
    ///
    /// 获取本地ip地址
    bool init();
    
    ///
    /// \brief 开启网络模块的线程
    ///
    bool startWork();
    
    ///
    /// \brief 结束网络模块各线程
    ///
    bool endWork();
    
    /// \brief 接受登陆并把节点放入cn_table 之后调用的ACK 返回
    /// \param client_socket 返回的计算节点的通信socket
    /// \param ack 返回的ack的内容
    /// \return 发送成功返回true
    bool tcpAckSend(TcpSocket *client_socket, LoginAck *ack);
    
    /// \brief 在dispatcher里面被send_task 调用
    /// \param client_ip 计算节点的IP
    /// \param task 发送的计算任务
    /// \return 成功则返回true, 失败false
    bool udpComputationTaskSend(const string client_ip, UdpSendTask *task);
    
    /// \brief 发送任务结束命令给计算节点, 在dispather 中调用
    /// \param client_ip 计算节点的IP
    /// \param end 发送的结束命令
    /// \return 成功则返回true, 失败false
    bool udpEndWork(const string client_ip, UdpEndWork *end);
    
private:
    ///
    /// \brief 删除初始化函数中的资源
    ///
    bool destroy();
    
    /// \brief 接受计算节点的登陆包
    ///
    /// 1.创建TCP socket进行监听
    /// 2.接受登陆包
    /// 3.将登陆包放入登陆节点最少的分组
    static void *tcpLoginRecv(void *arg);
    
    /// \brief 根据请求ID，发送原始任务
    ///
    /// 1.创建TCP socket进行监听
    /// 2.接受请求包
    /// 3.根据请求包发送原始任务
    static void *tcpOriginalTaskSend(void *arg);
    
    /// \brief 接受计算结果
    ///
    /// 1.创建UDP socket
    /// 2.接受结果包
    /// 3.根据结果包中的信息将结果包放入相应的队列
    static void *udpResultRecv(void *arg);

    /// \brief 接受命令系统的命令
    ///
    /// 1.创建TCP socket
    /// 2.接受命令
    /// 3.调用TaskManager的相应函数
    static void *tcpCommondRecv(void *arg);
    
    char* m_local_ip;                           ///< 调度节点本地IP
    pthread_t m_tcp_login_thread;               ///< 接受节点登陆的线程ID
    pthread_t m_tcp_original_task_thread;       ///< 接受原始任务请求线程ID
    pthread_t m_udp_result_recv_thread;         ///< 接受计算节点发送的结果线程ID
    pthread_t m_tcp_commond_recv_thread;
};


#endif
