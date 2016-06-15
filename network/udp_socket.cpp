#include "../network/udp_socket.h"

UdpSocket::UdpSocket() {
    this->m_udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_udp_socket_fd < 0) {
        LOG(ERROR) << "udpSocket Can't create new socket";
    }
}

UdpSocket::~UdpSocket() {
    this->udpClose();
}

int32 UdpSocket::getUdpSocket() const {
    return this->m_udp_socket_fd;
}

bool UdpSocket::udpBind(const char *host_ip, uint16 host_port) {
    
    struct sockaddr_in sa_server;
    bzero(&sa_server, sizeof(sockaddr_in));
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = htons(host_port);
    if (0 < inet_pton(AF_INET, host_ip, &sa_server.sin_addr) &&
        0 <= bind(m_udp_socket_fd, reinterpret_cast<sockaddr*>(&sa_server), sizeof(sockaddr_in))) {
        return true;
    }    
    LOG(ERROR) << "udpSocket Failed bind on" << host_ip << ":" << host_port;
    return false;

    /*
    struct sockaddr_in addr_ser;  
    bzero(&addr_ser,sizeof(addr_ser));  
    addr_ser.sin_family=AF_INET;  
    addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);  
    addr_ser.sin_port=htons(host_port);  
    int err=bind(m_udp_socket_fd,(struct sockaddr *)&addr_ser,sizeof(addr_ser));  
    if(err==-1)  
    {  
        printf("bind error:%s\n",strerror(errno));  
        return false;  
    }  
    return true;
    */
          
}

bool UdpSocket::udpClose() {
    if (this->m_udp_socket_fd > 0) {
        if (0 != close(m_udp_socket_fd)) {
            LOG(ERROR) << "udpSocket close socket failed";
            return false;
        }
        this->m_udp_socket_fd = -1;
    }
    return true;
}


int64 UdpSocket::udpReceive(void *buffer, uint32 buffer_size) {
    struct sockaddr_in client_sa;
    bzero(&client_sa, sizeof(client_sa));
    int client_sa_len = sizeof(struct sockaddr_in);
    int64 recv_len = recvfrom(this->m_udp_socket_fd, buffer, buffer_size, 0,
                              reinterpret_cast<sockaddr*>(&client_sa), (socklen_t*)&client_sa_len);

    if (-1 == recv_len) {
        LOG(ERROR) << "udpSocket recv failed";
    }
    return recv_len;
}

int64 UdpSocket::udpSend(const void *data, uint32 data_len, const char *dest_ip, uint16 dest_port) {
    
    struct sockaddr_in sa_dest_server;
    bzero(&sa_dest_server, sizeof(sockaddr_in));
    sa_dest_server.sin_family = AF_INET;
    sa_dest_server.sin_port = htons(dest_port);
    int64 send_len = 0;
    if (0 < inet_pton(AF_INET, dest_ip, &sa_dest_server.sin_addr)) {
        send_len = sendto(m_udp_socket_fd, data, data_len, 0,
                      reinterpret_cast<sockaddr*>(&sa_dest_server), sizeof(sockaddr_in));
        if (-1 == send_len) {
            LOG(ERROR) << "udpSocket recv failed";
        }
    }
    return send_len;
    

    /*
    struct sockaddr_in addr_ser;
    bzero(&addr_ser,sizeof(addr_ser));  
    addr_ser.sin_family=AF_INET;  
    addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);  
    addr_ser.sin_port=htons(dest_port);  
    int64 send_len = 0;
    send_len = sendto(m_udp_socket_fd, data, data_len, 0,
                    reinterpret_cast<sockaddr*>(&addr_ser), sizeof(sockaddr_in));
    if (-1 == send_len) {
        LOG(ERROR) << "udpSocket recv failed";
    }
    return send_len;
    */
    
}
