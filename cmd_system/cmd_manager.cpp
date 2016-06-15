#include "cmd_manager.h"
using namespace std;
bool CMDManager::startWork() {
    
	//connect();
}

bool CMDManager::connect() {
    this->tcp_connect_socket = new TcpSocket();
    if(!(tcp_connect_socket->tcpConnnet("192.168.1.100", TCP_CMD_PORT))) {
        LOG(ERROR) << " Connect failed ";
        return false;
    }
    cout << "connect succeed !" << endl;
    return true;
}

bool CMDManager::close() {
    if(!(tcp_connect_socket->tcpClose())) {
        LOG(ERROR) << " close failed ";
        return false;
    }
    delete this->tcp_connect_socket;
    cout << "close succeed !" << endl;
    return true;
}

int32 CMDManager::createTask(string task_file) {

    this->connect();
	ifstream task_in;
    task_in.open(task_file.c_str());
    if (!task_in) {
        LOG(ERROR) << "commond open task file error" << endl;
        return NULL;
    }
    task_in.seekg(0, ios::end);
    int32 file_size = task_in.tellg();
    task_in.seekg(0, ios::beg);
    cout << "file size " << file_size << endl;
    char buf[file_size + 1];
    memset(buf, 0, file_size + 1);
    task_in.read(buf, file_size);
    task_in.close();
    string sendstr(buf);
    int64 send_byte = tcp_connect_socket->tcpSend(sendstr.c_str(), sendstr.length());
    if (send_byte < 0) {
        cout << "tcp send commond error" << endl;
        return -1;
    }
    cout << "wait for server response..." << endl;
   
    char recv_task_ID[5];
    memset(recv_task_ID, 0, 5);
    
    int32 recv_byte = tcp_connect_socket->tcpRecieve(recv_task_ID, 5);
    if (recv_byte < 0) {
        cout << "tcp recv task id error " << endl;
        return -1;
    }
    cout << recv_task_ID << endl;
    this->close();
    
	
}

int32 CMDManager::searchTask(int32 task_ID) {

}
