#ifndef Asy_asynchronous__
#define Asy_asynchronous__

#include <sys/socket.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>

#include "../common/event.h"
#include "../common/common.h"


class Asynchronous {
public:
	Asynchronous() {};

	~Asynchronous() {
		this->destroy();
	}

	bool setupClient(const char *ip, uint16 port);

	int64 sendEvent(Event *event);
	
	bool closeClient();

private:
	bool destroy() {
		this->closeClient();
	}
	
	int32 socket_fd;
	struct sockaddr_in addr_ser;
};

#endif