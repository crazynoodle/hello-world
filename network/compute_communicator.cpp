#include "../network/compute_communicator.h"
#include "../dcr/system_init.h"


bool ComputeCommunicator::startWork() {
    //创建接受任务线程
    int32 err = pthread_create(&m_receive_task, NULL, receiveTask, this);
    if (err != 0) {
        LOG(ERROR) << " failed in create receiveTask thread ";
        return false;
    }
    //连接调度器
    int32 try_time = 0;
    while (!(this->connectToScheduler()) && try_time < dcr::g_getConfig()->getTryConnectTime()) {
        LOG(ERROR) << " connect to Scheduler Node failed! try again... ";
        try_time++;
        sleep(10);
    }
    if (try_time == dcr::g_getConfig()->getTryConnectTime()) {
        LOG(ERROR) << " connect failed! exit... ";
        return false;
    }
    //创建线程发送心跳
    err = pthread_create(&m_send_heart_beat, NULL, heartBeat, this);
    if (err != 0) {
        LOG(ERROR) << " failed in create heartBeat thread ";
        return false;
    }
    //创建线程监听结束包
    err = pthread_create(&m_end_work, NULL, receiveEndWork, this);
    if (err != 0) {
        LOG(ERROR) << " failed in create receiveEndWork ";
        return false;
    }
    
    return true;
}

bool ComputeCommunicator::endWork() {
    //pthread_join(m_end_work, NULL);
    //结束线程
    int32 err1 = pthread_cancel(m_receive_task);
    int32 err2 = pthread_cancel(m_send_heart_beat);
    if (err1 != 0) {
        LOG(ERROR) << "cancel receive task failed";
        return false;
    } else if (err2 != 0) {
        LOG(ERROR) << "cancel send heart beat failed";
        return false;
    }
    return true;
}

bool ComputeCommunicator::init() {
    this->m_task_queue = new FunctionalQueue();
    this->m_task_queue->init(dcr::g_getTaskQueueOfComputeNode().get(), FQ_TASK);
    this->m_reserve_info = new LoginAck();
    this->m_mutex = new Mutex();
    this->m_mutex->init();
    this->m_local_ip = new char[16];
    this->getLocalIP(m_local_ip);
    LOG(INFO) << " local ip: " << m_local_ip << endl;
    this->m_scheduler_ip = new char[16];
    this->getSchedulerIP(m_scheduler_ip);
    return true;
}

bool ComputeCommunicator::destroy() {
    delete this->m_task_queue;
    this->m_task_queue = NULL;
    delete this->m_reserve_info;
    this->m_reserve_info = NULL;
    delete this->m_mutex;
    this->m_mutex = NULL;
    delete [] this->m_scheduler_ip;
    m_scheduler_ip = NULL;
    delete [] this->m_local_ip;
    m_local_ip = NULL;
    return true;
}

FunctionalQueue* ComputeCommunicator::getTaskQueue() {
    return this->m_task_queue;
}


bool ComputeCommunicator::sendResult(Result *compute_result, UdpSendTask *udp_task) {
    CHECK_NOTNULL(compute_result);
    UdpSocket send_result_socket;
    UdpComputeResult* udp_result = (UdpComputeResult*)::operator new(dcr::g_getUdpResultSize());
    udp_result->packet_type = COMPUTE_RESULT;
    udp_result->head.group_seq = udp_task->task_head.group_seq;
    udp_result->head.compute_node_index = udp_task->task_head.compute_node_index;
    udp_result->head.mq_task_index = udp_task->task_head.mq_task_index;
    udp_result->head.task_ID = udp_task->task_head.task_ID;
    udp_result->head.original_task_ID = udp_task->task_head.original_task_ID;
    
    strcpy(udp_result->head.cn_ip, m_local_ip);
    udp_result->head.task_queue_len = this->m_task_queue->getLen();
    int32 offset = sizeof(UdpComputeResultHead) + sizeof(SchedUdpRecvType);
    memcpy((char*)udp_result + offset, (void*)compute_result, dcr::g_getComputeResultSize());
    int64 send_byte = send_result_socket.udpSend(udp_result, dcr::g_getUdpResultSize(),
                                                 m_scheduler_ip, UDP_RESULT_PORT);
    delete udp_result;
    if (send_byte < 0) {
        LOG(ERROR) << " Send error in result send ";
        return false;
    }
    return true;
}

OriginalTask* ComputeCommunicator::getOriginalTask(int32 original_task_ID) {
    this->m_mutex->Lock();
    map<int32, OriginalTask*>::iterator find_iter;
    find_iter= this->m_original_task.find(original_task_ID);
    OriginalTask *original_task = NULL;
    if (find_iter == m_original_task.end()) {
        TcpSocket request_socket;
        int32 try_time = 0;
        while(!(request_socket.tcpConnnet(m_scheduler_ip, TCP_ORIGINAL_TASK_PORT))) {
            try_time++;
            if (try_time > 10) {
                this->m_mutex->unLock();
                LOG(ERROR) << " Request new original task failed";
                return NULL;
            }
        }
        scoped_ptr<OriginalTaskRequest> packet(new OriginalTaskRequest());
        packet->original_task_ID = original_task_ID;
        int64 ret = request_socket.tcpSend(packet.get(), sizeof(OriginalTaskRequest));
        LOG(INFO) << " request original_task: " << original_task_ID;
        
        if (ret <= 0) {
            LOG(ERROR) << " task request send failed ";
            LOG(ERROR) << " code: " << errno << " msg: " << strerror(errno);
            this->m_mutex->unLock();
            return NULL;
        }
        
        original_task = dcr::g_createOriginalTask();
        int64 recv_byte = request_socket.tcpRecieve(original_task,
                                                    dcr::g_getOriginalTaskSize());
        if (recv_byte <= 0) {
            this->m_mutex->unLock();
            LOG(ERROR) << " Request new original task failed ";
            LOG(ERROR) << " code " << errno << " msg: " << strerror(errno);
            return NULL;
        }
        
        this->m_original_task.insert(make_pair(original_task_ID, original_task));
        LOG(INFO) << " Insert new original task ";
    } else {
        original_task = find_iter->second;
    }
    this->m_mutex->unLock();
    return original_task;
}


void* ComputeCommunicator::receiveTask(void* arg) {
    
    ComputeCommunicator* ptr = (ComputeCommunicator*)arg;
    
    struct sigaction action;
    sigPipeIgnore(action);
    
    UdpSocket server;
    CHECK(server.udpBind(ptr->m_local_ip, UDP_TASK_PORT));
    
    FunctionalQueue *task_queue = ptr->getTaskQueue();
    
    FunctionalQueue temp_task_queue;
    temp_task_queue.init(dcr::g_getTaskQueueOfComputeNode().get(), FQ_TASK_TEMP);
    
    while (true) {
        
        int move_len = temp_task_queue.tempAllocFromGlobalFree(1);
        
        if (move_len == 0) {
            sleep(2);
            continue;
        }
        
        int32 head_index = temp_task_queue.getHead();

        UdpSendTask *recv_task = (UdpSendTask*)(temp_task_queue.getMemoryBody(head_index));
        int64 recv_len = server.udpReceive(recv_task, dcr::g_getUdpTaskSize());
        if (-1 == recv_len) {
            temp_task_queue.tempReleaseToGlobalFree();
        }
        
        while (task_queue->getLen() == dcr::g_getConfig()->getTaskQueuePerNode()) {
            sleep(1);
        }
        
        task_queue->moveFromTemp(&temp_task_queue, temp_task_queue.getLen());
        pthread_testcancel();
        
    }
    
    LOG(INFO) << " exit task recv thread...";
    server.udpClose();
    
    return NULL;
}

void *ComputeCommunicator::receiveEndWork(void *arg) {
    ComputeCommunicator* ptr = (ComputeCommunicator*)arg;
    
    struct sigaction action;
    sigPipeIgnore(action);
    
    UdpSocket server;
    CHECK(server.udpBind(ptr->m_local_ip, UDP_END_WORK_PORT));
    
    UdpEndWork end;
    int64 recv_len = server.udpReceive(&end, sizeof(UdpEndWork));
    if (recv_len == -1) {
        LOG(ERROR) << " recieve end work failed";
    }
    
    if (end.endWork) {
        LOG(INFO) << " recieve end work command" ;
    }
    
    while (dcr::g_getComputeCommunicator()->getTaskQueue()->getLen() != 0) {
        sleep(10);
    }
    server.udpClose();
    LOG(INFO) << " finish computation! ";
    
    return NULL;
}

void ComputeCommunicator::getSchedulerIP(char* scheduler_ip) {
    memcpy(scheduler_ip, dcr::g_getConfig()->getSchedulerIP(), 16);
}

void* ComputeCommunicator::heartBeat(void* arg) {
    
    ComputeCommunicator* ptr = (ComputeCommunicator*)arg;
    
    struct sigaction action;
    sigPipeIgnore(action);
    
    FunctionalQueue* task_queue = dcr::g_getComputeCommunicator()->getTaskQueue();
    UdpSocket udp_send_socket;
    while (true) {
        UdpHeartbeat heart_beat;
        memset(&heart_beat, 0, sizeof(UdpHeartbeat));
        heart_beat.packet_type = HEART_BEAT;
        heart_beat.group_seq = ptr->m_reserve_info->group_index;
        heart_beat.compute_node_index = ptr->m_reserve_info->cn_index;
        heart_beat.task_queue_len = ptr->getTaskQueue()->getLen();
        memcpy(heart_beat.cn_ip, ptr->m_local_ip, 16);
        
        int64 send_bytes = udp_send_socket.udpSend(&heart_beat, sizeof(heart_beat),
                                                   ptr->m_scheduler_ip, UDP_RESULT_PORT);
        
        if (send_bytes < 0) {
            LOG(ERROR) << " send error in heatBeat send";
        }
        //Caculator::m_lock.Lock();
        //LOG(INFO) << " task_queue_len : " << task_queue->getLen();
        std::cout << " task queue len : " << task_queue->getLen() << std::endl;
        //Caculator::m_lock.unLock();
        //cout << "send hearbeat " << endl;
        pthread_testcancel();
        
        sleep(dcr::g_getConfig()->getHeartBeatTime());
    }
    LOG(INFO) << " exit heart beat thread...";
    udp_send_socket.udpClose();
    return NULL;
}

bool ComputeCommunicator::connectToScheduler() {
    
    TcpSocket tcp_connect_socket;
    if(!(tcp_connect_socket.tcpConnnet(m_scheduler_ip, TCP_LOGIN_PORT))) {
        LOG(ERROR) << " Connect failed ";
        return false;
    }
    
    LoginPacket login;
    memset(&login, 0, sizeof(LoginPacket));
    memset(login.client_ip, 0, 16);
    memcpy(login.client_ip, this->m_local_ip, 16);
    login.client_socket = NULL;
    int64 rev_byte = tcp_connect_socket.tcpSend(&login, sizeof(login));
    if (rev_byte < 0) {
        LOG(ERROR) << " Send Login Packet failed ";
        return false;
    }
    
    rev_byte = tcp_connect_socket.tcpRecieve(m_reserve_info, sizeof(LoginAck));
    
    if (rev_byte <= 0) {
        LOG(ERROR) << " Rev ack failed ";
        return false;
    }
    
    LOG(INFO) << " Connect sucessfully finish ";
    tcp_connect_socket.tcpClose();
    return true;
}


