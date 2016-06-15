/*!
 * \file communicator.h
 * \brief 网络通信模块的基类
 *
 * 主要的派生类是计算节点和调度节点的网络模块
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef DCR_communicator_h
#define DCR_communicator_h

#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <memory.h>
#include <arpa/inet.h>  
#include <sys/socket.h>  
#include <sys/ioctl.h>  
#include <net/if.h> 
///
/// \brief 网络通信模块基类
///
class Communicator {
public:
    Communicator() {}
    
    ~Communicator() {}
    
    virtual bool startWork() = 0;
    
    virtual bool endWork() = 0;
    
    ///
    /// \brief 忽略某些信号
    ///
    static void sigPipeIgnore(struct sigaction& action);
    
    ///
    /// \brief 获取本地IP
    ///
    static bool getLocalIP(char* ip);
};

#endif
