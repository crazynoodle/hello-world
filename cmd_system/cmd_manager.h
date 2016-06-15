#ifndef __DCR__cmd_manager__
#define __DCR__cmd_manager__

#include "../network/tcp_socket.h"
#include "../util/common.h"
#include "../network/network_packet.h"
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>

using namespace std;
class CMDManager {
public:
	//建立命令系统服务端
	//建立接收命令线程
	~CMDManager() {
		//delete tcp_connect_socket;
	}
	bool startWork();
	int32 createTask(string task_file);
	int32 searchTask(int32 task_ID);

private:
	TcpSocket *tcp_connect_socket;

	bool connect(); 
	bool close();
};
#endif
