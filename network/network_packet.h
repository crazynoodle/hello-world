/*!
 * \file network_packet.h
 * \brief 网络包
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef DCR_network_packet_h
#define DCR_network_packet_h

#include "../util/common.h"
#include "../network/tcp_socket.h"

#include <string>
using namespace std;

#define TCP_LOGIN_PORT 9501
#define TCP_ORIGINAL_TASK_PORT 9502
#define TCP_CMD_PORT 9503
#define UDP_TASK_PORT 9504
#define UDP_RESULT_PORT 9505
#define UDP_END_WORK_PORT 9506
#define TCP_MAX_CONNECTION 1024
#define UDP_ASYN_PORT 9201

/*
#define TCP_LOGIN_PORT 8001
#define TCP_ORIGINAL_TASK_PORT 8002
#define UDP_TASK_PORT 8003
#define UDP_RESULT_PORT 8004
#define UDP_END_WORK_PORT 8005
#define TCP_MAX_CONNECTION 1024
*/
///
/// \brief 计算节点登陆包
///
typedef struct LoginPacket {
    char client_ip[16];
    TcpSocket *client_socket;
} LoginPacket;

///
/// \brief 调度节点返回给计算节点登陆的ACK
///
typedef struct LoginAck {
    int32 group_index;
    int32 cn_index;
} LoginAck;

///
/// \brief 计算节点请求原始任务
///
typedef struct OriginalTaskRequest {
    int32 original_task_ID;
} OriginalTaskRequest;

///
/// \brief 调度节点发送计算任务的包头
///
typedef struct UdpSendTaskHead {
    int32 group_seq;
    int32 mq_task_index;
    int32 compute_node_index;
    int32 original_task_ID;
    int32 task_ID;
} UdpSendTaskHead;


///
/// \brief 调度节点发送的计算任务包
///
typedef struct UdpSendTask {
    UdpSendTaskHead task_head;
    void *computation_task;
} UdpSendTask;

///
/// \brief 计算节点发送结果包的包头
///
typedef struct UdpComputeResultHead {
    int32 group_seq;
    int32 mq_task_index;
    int32 compute_node_index;
    int32 original_task_ID;
    int32 task_queue_len;
    char cn_ip[16];
    int32 task_ID;
} UdpComputeResultHead;

///
/// \brief 结果包的类型(包括心跳包和计算结果包)
///
typedef enum SchedUdpRecvType {
    COMPUTE_RESULT,
    HEART_BEAT
}SchedUdpRecvType;

///
/// \brief 计算节点发送的计算结果包
///
typedef struct UdpComputeResult {
    SchedUdpRecvType packet_type;
    UdpComputeResultHead head;
    void *compute_result;
} UdpComputeResult;

///
/// \brief 计算节点发送的心跳包
///
typedef struct UdpHeartbeat {
    SchedUdpRecvType packet_type;
    int32 group_seq;
    int32 compute_node_index;
    int32 task_queue_len;
    char cn_ip[16];
} UdpHeartbeat;

///
/// \brief 结果包
///
typedef union SchedUdpRecv {
    UdpComputeResult compute_result;
    UdpHeartbeat heartbeat;
} SchedUdpRecv;

///
/// \brief 任务结束的命令包
///
typedef struct UdpEndWork {
    bool endWork;
} UdpEndWork;

#endif
