/*!
 * \file udp_socket.h
 * \brief 封装udp socket 及相应操作
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef __DCR__udp_socket__
#define __DCR__udp_socket__

#include "../util/common.h"
#include "../util/logger.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>

///
/// \brief 封装udp socket及相应操作, 构造函数即创建了一个socket
///
class UdpSocket {
public:
    UdpSocket();
    
    ~UdpSocket();
    
    ///
    /// \brief 获取udp_socket 的文件描述法
    ///
    int32 getUdpSocket() const;
    
    /// \brief socket 与IP和端口号绑定, 用于接受数据的时候
    /// \param ip 绑定的IP
    /// \param port 绑定的端口
    /// \return 绑定成功则返回true
    bool udpBind(const char *ip, uint16 port);
    
    /// \brief 关闭socket
    /// \return 关闭成功则返回true, 失败则false
    bool udpClose();
    
    /// \brief 用已经创建的socket 接受数据
    /// \param buffer 用于接收数据的缓冲
    /// \param buffer_size 要接受数据的大小
    /// \return 接受的字节数
    int64 udpReceive(void *buffer, uint32 buffer_size);
    
    /// \brief 用已经创建的socket 发送数据
    /// \param data 要发送的数据
    /// \param data_len 要发送的数据的大小
    /// \param dest_ip 目的IP
    /// \param dest_port 目的端口
    /// \return 实际发送数据字节数
    int64 udpSend(const void *data, uint32 data_len, const char *dest_ip, uint16 dest_port);
    
private:
    int32 m_udp_socket_fd;  ///< 获取socket文件描述符
};

#endif
