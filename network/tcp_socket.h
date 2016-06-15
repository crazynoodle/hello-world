/*!
 * \file tcp_socket.h
 * \brief 封装tcp socket 及相应操作
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef __DCR__tcp_socket__
#define __DCR__tcp_socket__

#include "../util/common.h"
#include "../util/logger.h"

#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
///
/// \brief 封装tcp socket及相应操作, 构造函数即创建了一个socket
///
class TcpSocket {
public:
    TcpSocket();
    
    ~TcpSocket();
    
    ///
    /// \brief 获取udp_socket 的文件描述法
    ///
    int32 getTcpSocketFd() const;
    
    void setTcpSocketFd(int32 tcp_socket_fd);
    
    /// \brief 作为客户端的时候发起连接
    /// \param ip 服务端IP
    /// \param port 通信端口
    /// \return 建立连接成功还是失败
    bool tcpConnnet(const char *ip, uint16 port);
    
    /// \brief 作为服务端的绑定IP和端口
    /// \param ip 服务端IP
    /// \param port 通信端口
    /// \return 绑定成功还是失败
    bool tcpBind(const char *ip, uint16 port);
    
    /// \brief 服务端监听端口
    /// \param max_connection 等待队列最大连接数
    /// \return 监听失败还是成功
    bool tcpListen(int32 max_connection);
    
    /// \brief 服务端接受客户端连接
    /// \param client_socket 接受连接后与客户端通信的socket
    /// \return 接受连接失败还是成功
    bool tcpAccept(TcpSocket *client_socket);
    
    ///
    /// \brief 设置阻塞
    ///
    bool tcpSetBlocking(bool flag);
    
    ///
    /// \brief 单向关闭连接, 双向关闭连接
    ///
    bool tcpShutdown(int32 way);
    
    ///
    /// \brief 关闭socket
    ///
    bool tcpClose();
    
    /// \brief 用已经创建的socket 接受数据
    /// \param buffer 用于接收数据的缓冲
    /// \param buffer_size 要接受数据的大小
    /// \return 接受的字节数
    int64 tcpRecieve(void *buffer, int64 buffer_size);
    
    /// \brief 用已经创建的socket 发送数据
    /// \param data 用于发送数据的缓冲
    /// \param data_len 要发送数据的大小
    /// \return 发送的字节数
    int64 tcpSend(const void *data, int64 data_len);
    
private:
    int32 m_tcp_socket_fd;   ///< 创建的TCP socket 的文件描述内容
};

#endif