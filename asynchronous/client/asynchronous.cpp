#include "asynchronous.h"
using namespace std;

bool Asynchronous::setupClient(const char *ip, uint16 port) {    
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_fd == -1) {  
    	cout << "socket error " << endl;
        return false;  
    }
    bzero(&addr_ser, sizeof(addr_ser));  
    addr_ser.sin_family = AF_INET;  
    addr_ser.sin_addr.s_addr = inet_addr(ip);
    addr_ser.sin_port = htons(port);  
    cout << "setup successful " << endl;
    return true;    
}
	
int64 Asynchronous::sendEvent(Event *event) {
    struct sockaddr_in sa_dest_server;
	string json_event = event->toJson();
	int64 send_len = sendto(socket_fd, json_event.c_str(), json_event.length()+1, 0,
                            reinterpret_cast<sockaddr*>(&addr_ser), sizeof(sockaddr_in));
	if (send_len == -1) {
        cout << "send event failed " << endl;
    }
    return send_len;

}

bool Asynchronous::closeClient() {
	if (this->socket_fd >= 0) {
        if (0 != close(this->socket_fd)) {
            cout << "event Close failed " << endl;
            return false;
        }
        this->socket_fd = -1;
    }
    cout << "close client!" << endl;
    return true;
	
}