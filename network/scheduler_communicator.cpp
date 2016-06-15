#include "../network/scheduler_communicator.h"
#include "../util/scoped_ptr.h"
#include "../util/logger.h"
#include "../core_data_structure/compute_node_table.h"
#include "../dcr/dcr_base_class.h"
#include "../dcr/system_init.h"
#include <fstream>

bool SchedulerCommunicator::init() {
    // 获取本地ip地址
    m_local_ip = new char[16];
    getLocalIP(m_local_ip);
    LOG(INFO) << " local ip : " << m_local_ip;
    return true;
}

bool SchedulerCommunicator::destroy() {
    delete []m_local_ip;
    m_local_ip = NULL;
    return true;
}

bool SchedulerCommunicator::startWork() {
    //创建一个线程用于接受节点登录
    int32 retval = pthread_create(&m_tcp_login_thread, NULL, tcpLoginRecv, this);
    if (retval == -1) {
        LOG(ERROR) << "create login thread failed";
        return false;
    }
    //创建一个线程接受原始任务请求
    retval =  pthread_create(&m_tcp_original_task_thread, NULL, tcpOriginalTaskSend, this);
    if (retval == -1) {
        LOG(ERROR) << "create original_task_thread failed";
        return false;
    }
    //创建一个线程接受计算节点发送的结果
    retval = pthread_create(&m_udp_result_recv_thread, NULL, udpResultRecv, this);
    if (retval == -1) {
        LOG(ERROR) << "create result_recv thread failed";
        return false;
    }
    //TODO: 创建线程接受命令？？？
    retval = pthread_create(&m_tcp_commond_recv_thread, NULL, tcpCommondRecv, this);
    if (retval == -1) {
        LOG(ERROR) << "create commond_recv thread failed";
        return false;
    }

    return true;
}

bool SchedulerCommunicator::endWork() {
    int32 err1 = pthread_cancel(m_tcp_login_thread);
    int32 err2 = pthread_cancel(m_tcp_original_task_thread);
    int32 err3 = pthread_cancel(m_udp_result_recv_thread);
    int32 err4 = pthread_cancel(m_tcp_commond_recv_thread);
    if (err1 != 0) {
        LOG(ERROR) << "cancel login thread failed";
        return false;
    } else {
        if (err2 != 0) {
            LOG(ERROR) << "cancel original task thread failed";
            return false;
        } else if (err3 != 0) {
            LOG(ERROR) << "cancel result recv thread failed";
            return false;
        }
    }
    if(err4 != 0) {
         LOG(ERROR) << "cancel commond recv thread failed";
    }
    
    return true;
}

void *SchedulerCommunicator::tcpLoginRecv(void *arg) {
    SchedulerCommunicator* ptr = (SchedulerCommunicator*)arg;
    
    //临时接受队列
    FunctionalQueue temp_tcp_recv;
    temp_tcp_recv.init(dcr::g_getLoginQueue().get(), FQ_TCP_RECV_TEMP);
    
    struct sigaction action;
    sigPipeIgnore(action);
    
    //创建tcpserver
    TcpSocket server;
    CHECK(server.tcpBind(ptr->m_local_ip, TCP_LOGIN_PORT));
    CHECK(server.tcpListen(TCP_MAX_CONNECTION));
    
    while (true) {
        //接收请求
        TcpSocket *client_socket = new TcpSocket();
        if (!(server.tcpAccept(client_socket))) {
            continue;
        }
        //接受到新的登录请求
        LOG(INFO) << " accept new node login! ";
        //申请内存
        int32 alloc_tcp_recv_num = temp_tcp_recv.tempAllocFromGlobalFree(1);
        if (alloc_tcp_recv_num != 1) {
            temp_tcp_recv.tempReleaseToGlobalFree();
            LOG(ERROR) << "alloc tcp_recv_num failed";
            continue;
        }
        //接受一个loginPacket
        int32 head_index = temp_tcp_recv.getHead();
        LoginPacket *rev_login = (LoginPacket*)(temp_tcp_recv.getMemoryBody(head_index));
        int64 recv_len = client_socket->tcpRecieve(rev_login, sizeof(LoginPacket));
        
        if (recv_len < 0) {
            temp_tcp_recv.tempReleaseToGlobalFree();
            continue;
        }
        //设置登录请求包的client_socket为当前客户端socket
        rev_login->client_socket = client_socket;
        
        //group_seq为要登录节点表分组，初始化为0
        int32 group_seq = 0;
        //cn_num_min为节点表分组中最小的节点数，初始化为登录队列最大的长度
        int32 cn_num_min = dcr::g_getConfig()->getLoginQueueLen();
        //节点表的分组数
        int32 group_num = dcr::g_getComputeNodeTable().get()->getGroupNum();
        GroupInTable *group_in_table = NULL;
        int32 active_len = 0;
        int32 login_len = 0;
        //遍历节点表中的每个分组，找到节点数最少的分组用于登录
        for (int32 i = 0; i < group_num; i++) {
            //获取分组
            group_in_table = dcr::g_getComputeNodeTable().get()->getGroup(i);
            //有效节点数
            active_len = group_in_table->getActiveComputeNode()->getLen();
            //登录节点数
            login_len = group_in_table->getLogin()->getLen();
            //如果已知节点分组的最小节点数cn_num_min大于当前分组的有效节点数+登录队列长度
            //则将group_seq（要登录的分组）设置为当前分组
            //并将cn_num_min设置为当前分组的有效节点数+登录队列长度
            //最后可以找到节点数最少的分组
            if(cn_num_min > (active_len + login_len)) {
                group_seq = group_in_table->getActiveComputeNode()->getGroupSeq();
                cn_num_min = active_len + login_len;
            }
            
        }
        //通过group_seq获取到节点最少的分组
        group_in_table = dcr::g_getComputeNodeTable().get()->getGroup(group_seq);
        //获取该分组的登录队列
        FunctionalQueue *login_in_group = group_in_table->getLogin();
        //将接受到的包放入改分组的登录队列
        login_in_group->moveFromTemp(&temp_tcp_recv, temp_tcp_recv.getLen());
        
        pthread_testcancel();
    }
    
    LOG(INFO) << "exit login recv thread...";
    //关闭连接
    server.tcpClose();
    return (void*)NULL;
}


void *SchedulerCommunicator::tcpOriginalTaskSend(void *arg) {
    
    SchedulerCommunicator* ptr = (SchedulerCommunicator*)arg;
    
    struct sigaction action;
    sigPipeIgnore(action);
    //创建tcpserver
    TcpSocket server;
    CHECK(server.tcpBind(ptr->m_local_ip, TCP_ORIGINAL_TASK_PORT));
    CHECK(server.tcpListen(TCP_MAX_CONNECTION));
    
    OriginalTask *original_task = NULL;
    scoped_ptr<OriginalTaskRequest> request(new OriginalTaskRequest());
    while (true) {
        //接收请求
        TcpSocket client_socket;
        if (!(server.tcpAccept(&client_socket))) {
            continue;
        }
        int64 recv_len = client_socket.tcpRecieve(request.get(), sizeof(OriginalTaskRequest));
        if (recv_len == -1) {
            LOG(ERROR) << " recv original_task request failed";
            LOG(ERROR) << " code: " << errno << " msg: " << strerror(errno);
            continue;
        }
        //根据请求的任务id从分解器获取到原来的任务
        original_task = dcr::g_getDecomposer().get()->getOriginalTaskByID(request->original_task_ID);
        CHECK_NOTNULL(original_task);
        //发送原来的任务给client
        int64 ret = client_socket.tcpSend(original_task, dcr::g_getOriginalTaskSize());
        if (ret <= 0) {
            LOG(ERROR) << " send original task failed";
        }
        //关闭与client的连接
        client_socket.tcpClose();
        
        pthread_testcancel();
    }
    LOG(INFO) << "exit original thread..." << endl;
    server.tcpClose();
    
    return (void*)NULL;
}



void *SchedulerCommunicator::udpResultRecv(void *arg) {
    
    SchedulerCommunicator* ptr = (SchedulerCommunicator*)arg;
    //临时结果接收队列
    FunctionalQueue temp_udp_recv;
    temp_udp_recv.init(dcr::g_getResultQueue().get(), FQ_UDP_RECV_TEMP);
    
    struct sigaction action;
    sigPipeIgnore(action);
    //创建udpserver
    UdpSocket server;
    CHECK(server.udpBind(ptr->m_local_ip, UDP_RESULT_PORT));
    
    while (true) {
        //申请内存
        int32 alloc_udp_result_num = temp_udp_recv.tempAllocFromGlobalFree(1);
        if (alloc_udp_result_num != 1) {
            temp_udp_recv.tempReleaseToGlobalFree();
            LOG(ERROR) << "alloc udp_result_num failed";
            continue;
        }
        //获取udp请求
        int32 head_index = temp_udp_recv.getHead();
        SchedUdpRecv *recv_packet = (SchedUdpRecv*)temp_udp_recv.getMemoryBody(head_index);
        int64 recv_len = server.udpReceive(recv_packet, dcr::g_getUdpResultSize());
        if (-1 == recv_len) {
            temp_udp_recv.tempReleaseToGlobalFree();
            continue;
        }
        //group_seq为请求的分组
        int group_seq = 0;
        //获取请求的来源分组
        if (recv_packet->compute_result.packet_type == HEART_BEAT) {
            //心跳包
            group_seq = recv_packet->heartbeat.group_seq;
        } else {
            //计算结果
            group_seq = recv_packet->compute_result.head.group_seq;
        }
        //从节点表获取group_seq对应的分组
        GroupInTable *group_in_table = dcr::g_getComputeNodeTable().get()->getGroup(group_seq);
        FunctionalQueue *result_queue = group_in_table->getResult();
        //将结果放入结果队列
        result_queue->moveFromTemp(&temp_udp_recv, temp_udp_recv.getLen());
        
        pthread_testcancel();
    }
    LOG(INFO) << "exit result recv thread...";
    server.udpClose();
    return (void*)NULL;
}
/*
void *SchedulerCommunicator::tcpCommondRecv(void *arg) {

    SchedulerCommunicator* ptr = (SchedulerCommunicator*)arg;
    
    struct sigaction action;
    sigPipeIgnore(action);
    
    TcpSocket server;
    CHECK(server.tcpBind(ptr->m_local_ip, TCP_CMD_PORT));
    CHECK(server.tcpListen(TCP_MAX_CONNECTION));
    const int64 MAX_LEN = 1024*10;
    

    while (true) {
        TcpSocket *client_socket = new TcpSocket();
        if (!(server.tcpAccept(client_socket))) {
            continue;
        }
        cout << " recv a commond " << endl;
        
        char commond;
        if (client_socket->tcpRecieve(&commond, sizeof(char)) > 0) {

            switch(commond) {
                case '1':
                {
                    cout << "create task " << endl;
                    int recv_len, templen = 0;
                    char tempbuf[1025];
                    if ((recv_len = client_socket->tcpRecieve(tempbuf, sizeof(char)*1024)) > 0) {
                        char *buf = new char[MAX_LEN];
                        memset(buf, 0, MAX_LEN);
                        while((recv_len > 0)&&((templen + recv_len) < MAX_LEN)) {
                            memcpy(buf + templen, tempbuf, recv_len);
                            templen += recv_len;
                            //cout << buf << endl;
                            memset(tempbuf, 0, 1025);
                            recv_len = client_socket->tcpRecieve(tempbuf, sizeof(char)*1024);
                        }

                        cout << " recv a new task! " << endl; 
                        cout << buf << endl;
                        int32 task_ID = dcr::g_getTaskManager()->create_task(buf + 1, templen - 1);
                        delete[] buf;

                        char task_buf[20];
                        memset(task_buf, 0, 20);
                        sprintf(task_buf, "%lli", task_ID);
                        int32 send_len = client_socket->tcpSend(task_buf, strlen(task_buf) + 1);
                        cout << "send len is " << send_len << endl;
                    }
                   
                    break;
                }
                case '6':
                {
                    cout << "commond 6" << endl;
                    break;
                }
                default:
                {
                    cout << "commond error" << endl; 
                    break;
                }
            }
            
        }
        client_socket->tcpClose();
        delete client_socket;
        pthread_testcancel();

    } 

    LOG(INFO) << "exit login recv thread...";
    server.tcpClose();   
        
}*/

#define MAX_GPR_SIZE 102400
void* do_interface(void *newsockfd)
{
    uint64 taskid;
    char task_buf[20];
    memset(task_buf, 0, 20);
    int rc;
    char command;
    //如果接收到command
    if(recv(*(int*)newsockfd,&command,sizeof(char),0) > 0)
    {
        switch(command)
        {
            //command为1
            case '1':
                {

                    char add_pos = 0;
                    //继续接受command并设置add_pos
                    if(recv(*(int*)newsockfd,&command,sizeof(char),0) > 0)
                    {
                        switch(command)
                        {
                            case '0':
                                add_pos = '0';
                                break;
                            case '1':
                                add_pos = '1';
                                break;
                            default:
                                cout<<"There maybe something wrong while transfering add_pos!"<<endl;
                        }
                    }
                    else
                    {
                        //接受command失败返回-2
                        send(*(int*)newsockfd,"-2",3,0);
                        break;
                    }
                    
                    int recv_len, templen = 0;
                    //临时缓冲区
                    char tempbuf[1025];
                    memset(tempbuf, 0, 1025);
                    struct timeval timeout = {3, 0};
                    setsockopt(*(int*)newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
                    //如果接收到数据
                    if( (recv_len = recv(*(int*)newsockfd,tempbuf, sizeof(char)*1024,0))>0)
                    {   
                        //接收数据
                        char *buf = new char[MAX_GPR_SIZE];
                        memset(buf,0,MAX_GPR_SIZE);
                        while((recv_len > 0)&&((templen+recv_len) < MAX_GPR_SIZE))
                        {
                            memcpy(buf+templen, tempbuf, recv_len);
                            templen += recv_len;

                            memset(tempbuf, 0, 1025);
                            recv_len = recv(*(int*)newsockfd,tempbuf, sizeof(char)*1024,0);
                        }
                        //cout << buf << endl;
                        //用任务管理器(TaskManager)创建任务，并获取任务id
                        int32 task_ID = dcr::g_getTaskManager()->create_task(buf, templen);
                        delete[] buf;

                        sprintf(task_buf,"%lli",task_ID);
                        //返回任务id
                        send(*(int*)newsockfd,task_buf,strlen(task_buf) + 1, 0);

                        memset(task_buf, 0, 20);
                    }
                    else
                    {
                        send(*(int*)newsockfd,"-2", 3, 0);
                    }
                    break;
                }
        }
    }
    //关闭socket
    close(*(int*)newsockfd);
    return NULL;
    //结束线程
    pthread_exit(NULL);
}

void *SchedulerCommunicator::tcpCommondRecv(void *arg) {
    int sockfd,newsockfd;
    struct sigaction action;
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPIPE, &action, NULL);
    struct sockaddr_in server ={AF_INET,htons(TCP_CMD_PORT),{INADDR_ANY}};
    bzero(&(server.sin_zero),8); 
    //创建tcpserver，这里调用的是原生的socket api
    //调用socket
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        perror("SOCKET CALL failed");
        return NULL;
    }
    unsigned int value = 0x1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&value ,sizeof(value));
    //调用bind
    if(bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1)
    {
        perror("bind call failed");
        return NULL;
    }
    //调用listen
    if(listen(sockfd,50) == -1)
    {
        perror("listen call failed");
        return NULL;
    }
    while(true)
    {
        //接受请求
        if((newsockfd = accept(sockfd,NULL,NULL)) == -1)
        {
            perror("accept call failed");
            continue;
        }
        //创建新的线程，执行do_interface
        pthread_t do_inter;
        pthread_create(&do_inter, NULL, do_interface, (void*)&newsockfd);
        pthread_join(do_inter,NULL);
    }
}


bool SchedulerCommunicator::tcpAckSend(TcpSocket *client_socket, LoginAck *ack) {
    int64 send_bytes = client_socket->tcpSend(ack, sizeof(LoginAck));
    if (send_bytes < 0) {
        LOG(ERROR) << "errno : " << strerror(errno);
        LOG(ERROR) << "send ack failed";
        return false;
    }
    return true;
}

bool SchedulerCommunicator::udpComputationTaskSend(const string client_ip, UdpSendTask *task) {
    CHECK_NOTNULL(task);
    UdpSocket udp_send_socket;
    int64 send_bytes = udp_send_socket.udpSend(task, dcr::g_getUdpTaskSize(),
                                               client_ip.c_str(), UDP_TASK_PORT);
    if (send_bytes < 0 ) {
        LOG(ERROR) << "send error in task send";
        return false;
    }
    return true;
}

bool SchedulerCommunicator::udpEndWork(const string client_ip, UdpEndWork *end) {
    CHECK_NOTNULL(end);
    UdpSocket udp_send_socket;
    int64 send_bytes = udp_send_socket.udpSend(end, sizeof(UdpEndWork),
                                               client_ip.c_str(), UDP_END_WORK_PORT);
    LOG(INFO) << " send end work " << send_bytes;
    
    if (send_bytes < 0) {
        LOG(ERROR) << "send error in end work";
        return false;
    }
    return true;
}


