#include "../network/tcp_socket.h"

TcpSocket::TcpSocket() {
    this->m_tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_tcp_socket_fd < 0) {
        LOG(FATAL) << " tcpSocket Can't create new socket ";
    }
}

TcpSocket::~TcpSocket() {
    this->tcpClose();
}

int32 TcpSocket::getTcpSocketFd() const {
    return this->m_tcp_socket_fd;
}

void TcpSocket::setTcpSocketFd(int32 tcp_socket_fd) {
    using namespace std;
    CHECK_GE(tcp_socket_fd, 0);
    this->m_tcp_socket_fd = tcp_socket_fd;
}

bool TcpSocket::tcpConnnet(const char *dest_ip, uint16 dest_port) {
    sockaddr_in sa_server;
    bzero(&sa_server, sizeof(sa_server));
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = htons(dest_port);
    if (0 < inet_pton(AF_INET, dest_ip, &sa_server.sin_addr) &&
        0 <= connect(m_tcp_socket_fd, reinterpret_cast<sockaddr*>(&sa_server), sizeof(sa_server))) {
        return true;
    }
    using namespace std;
    cout << "socket error!"<<strerror(errno)<<endl;
    LOG(ERROR) << " tcpSocket failed connect to " << dest_ip << ":" << dest_port;
    return false;

    /*
    struct sockaddr_in addr_ser;  
    bzero(&addr_ser,sizeof(addr_ser));  
    addr_ser.sin_family=AF_INET;  
    addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);  
    addr_ser.sin_port=htons(dest_port);  
    int err=connect(m_tcp_socket_fd,(struct sockaddr *)&addr_ser,sizeof(addr_ser));  
    if(err==-1)  
    {  
        using namespace std;
        cout << "socket error!"<<strerror(errno)<<endl;
        return false;  
    }  
    return true;
    */
}

bool TcpSocket::tcpBind(const char *host_ip, uint16 host_port) {
    
    sockaddr_in sa_server;
    bzero(&sa_server, sizeof(sockaddr_in));
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = htons(host_port);
    
    if (0 < inet_pton(AF_INET, host_ip, &sa_server.sin_addr) &&
        0 <= bind(m_tcp_socket_fd, reinterpret_cast<sockaddr*>(&sa_server), sizeof(sa_server))) {
        return true;
    }
    using namespace std;
    cout << "socket error!"<<strerror(errno)<<endl;
    LOG(ERROR) << " tcpSocket Failed bind on " << host_ip << ":" << host_port;
    return false;
    
   
    /*
    struct sockaddr_in ser_addr; 
    bzero(&ser_addr,sizeof(ser_addr));  
    ser_addr.sin_family=AF_INET;  
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
    ser_addr.sin_port=htons(host_port);  
    int err=bind(m_tcp_socket_fd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));  
    if(err==-1)  
    {  
        using namespace std;
        cout << "socket error!"<<strerror(errno)<<endl;
        return false;  
    }  
    return true;
    */
}

bool TcpSocket::tcpListen(int32 max_connection) {
    if (0 <= listen(m_tcp_socket_fd, max_connection)) {
        return true;
    }
    LOG(ERROR) << " tcpSocket failed listen on socket fd: " << m_tcp_socket_fd;
    return false;
}

bool TcpSocket::tcpAccept(TcpSocket *client_socket) {
    int32 client_socket_fd;
    sockaddr_in client_sa;
    bzero(&client_sa, sizeof(client_sa));
    socklen_t len = sizeof(client_sa);
    
    client_socket_fd = accept(m_tcp_socket_fd, reinterpret_cast<sockaddr*>(&client_sa), &len);
    if (client_socket < 0) {
        LOG(ERROR) << " tcpSocket failed accept connection ";
        return false;
    } else {
        char temp[INET_ADDRSTRLEN];
        const char *client_ip = inet_ntop(AF_INET, &client_sa.sin_addr, temp, sizeof(temp));
        //LOG(INFO) << " accept from : " << client_ip;
    }
    
    client_socket->m_tcp_socket_fd = client_socket_fd;
    return true;
    
}


bool TcpSocket::tcpSetBlocking(bool flag) {
    int32 opts;
    
    if ((opts = fcntl(this->m_tcp_socket_fd, F_GETFL))) {
        LOG(ERROR) << " tcpSocket Failed to get socket status. ";
        return false;
    }
    
    if (flag) {
        opts |= O_NONBLOCK;
    } else {
        opts &= ~O_NONBLOCK;
    }
    
    
    if (fcntl(this->m_tcp_socket_fd, F_SETFL, opts)) {
        LOG(ERROR) << " tcpSocket failed to set socket status ";
        return false;
    }
    
    return true;
}


bool TcpSocket::tcpShutdown(int32 ways) {
    return 0 == shutdown(this->m_tcp_socket_fd, ways);
}

bool TcpSocket::tcpClose() {
    if (this->m_tcp_socket_fd >= 0) {
        if (0 != close(this->m_tcp_socket_fd)) {
            LOG(ERROR) << " tcpSocket Close failed ";
            return false;
        }
        this->m_tcp_socket_fd = -1;
    }
    return true;
}


int64 TcpSocket::tcpSend(const void *data, int64 data_len) {
    int64 send_len = send(this->m_tcp_socket_fd, data, data_len, 0);
    if (send_len == -1) {
        LOG(ERROR) << " Tcpsocket send data failed ";
    }
    return send_len;
}

int64 TcpSocket::tcpRecieve(void *buffer, int64 buffer_size) {
    int64 recv_len = recv(this->m_tcp_socket_fd, buffer, buffer_size, 0);
    if (recv_len < 0) {
        LOG(ERROR) << " TcpSocket recv data failed ";
    }
    return recv_len;
}
