#include "strategy.h"
#include "../util/logger.h"
#include "../util/mutex.h"
#include "../util/scoped_ptr.h"
#include "../network/scheduler_communicator.h"
#include "../dcr/system_init.h"
#include "../dcr/dispacher.h"
#include "../dcr/task_manager.h"

Mutex Dispatcher::m_lock;
//int32 task_ID = 0;
int32 redispatch_index = 0;

void Strategy::decomposeStrategy(Decomposer* ptr) {
    int32 task_ID = 0;
    FunctionalQueue temp_decomposer_queue;
    temp_decomposer_queue.init(dcr::g_getComputationTaskQueue().get(), FQ_DECOMPOSER_TEMP);
    scoped_ptr<ComputationTask> new_task;

    while (true) {
        Task *new_job_task = dcr::g_getTaskManager()->getNextTask();
        if (NULL == new_job_task) {
            sleep(5);
            continue;
        }
        int32 original_ID = ptr->decomposeNewTask(new_job_task);
        if (original_ID == -1) {
            LOG(ERROR) << " decompoes new task fail";
            return;
        }

        int32 computeSpeed = ptr->computeDecomposeSpeed();
        temp_decomposer_queue.tempAllocFromGlobalFree(computeSpeed);
        int32 temp_queue_index = temp_decomposer_queue.getHead();
        
        UdpSendTask *body = static_cast<UdpSendTask*>(temp_decomposer_queue.getMemoryBody(temp_queue_index));

        string result_path = ptr->m_id_resultName[original_ID];
        OriginalTask *ori_task = ptr->getOriginalTaskByID(original_ID);
        Granularity *granu = ptr->getGranulityByID(original_ID);
        //更新结果文件
        //dcr::g_getJob()->updateResultFileByDecomposer(ori_task, original_ID, granu, ptr, result_path, true);
        
        //第一次因为要进行初始化所以要独立开来
        new_task.reset(ptr->getTask(original_ID, ori_task, granu, true));
        
        int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
        memcpy(((char*)body + offset), new_task.get(), dcr::g_getComputeTaskSize());
        
        body->task_head.original_task_ID = original_ID;
        body->task_head.task_ID = task_ID++;
        int32 already_decompose = 1;
        
        int32 next_index = temp_decomposer_queue.getFqNextByIndex(temp_queue_index);
        temp_queue_index = next_index;

        while (!(ptr->isFinishGetTask())) {
            if (already_decompose >= computeSpeed) {
                ptr->m_decompose_queue->moveFromTemp(&temp_decomposer_queue,
                                                     temp_decomposer_queue.getLen());
                temp_decomposer_queue.tempReleaseToGlobalFree();
                ptr->m_decompose_num[original_ID] += already_decompose;
                already_decompose = 0;
                computeSpeed = ptr->computeDecomposeSpeed();
                temp_decomposer_queue.tempAllocFromGlobalFree(computeSpeed);
                temp_queue_index = temp_decomposer_queue.getHead();
                sleep(dcr::g_getConfig()->getSchedulerTime());
            }

        
            //dcr::g_getJob()->updateResultFileByDecomposer(ori_task, original_ID, granu, ptr, result_path, false);

            body = static_cast<UdpSendTask*>(temp_decomposer_queue.getMemoryBody(temp_queue_index));
            new_task.reset(ptr->getTask(original_ID, ori_task, granu, false));
            memcpy(((char*)body + offset), new_task.get(), dcr::g_getComputeTaskSize());
            body->task_head.original_task_ID = original_ID;
            body->task_head.task_ID = task_ID++;
            already_decompose++;
            
            next_index = temp_decomposer_queue.getFqNextByIndex(temp_queue_index);
            temp_queue_index = next_index;
        }
        
        ptr->m_decompose_num[original_ID] += already_decompose;
        //循环结束， 把最后一次的分解任务进行处理, 并将分解了多少个任务的信息保存起来
        ptr->m_decompose_queue->moveFromTemp(&temp_decomposer_queue,
                                             already_decompose);
        temp_decomposer_queue.tempReleaseToGlobalFree();
        redispatch_index = task_ID;
        new_job_task->decompose_packet_num =  ptr->m_decompose_num[original_ID];
        //LOG(WARNING) << " decompoes num is " <<  new_job_task->decompose_packet_num;
    
    }
}

void Strategy::dispachStrategy(int group_seq) {
    GroupInTable *group_in_table = dcr::g_getComputeNodeTable()->getGroup(group_seq);
    FunctionalQueue *active_compute_node = group_in_table->getActiveComputeNode();
    FunctionalQueue *group_task_free = group_in_table->getGroupFree();
    FunctionalQueue *login = group_in_table->getLogin();
    FunctionalQueue *result = group_in_table->getResult();
    FunctionalQueue *decompose_task = dcr::g_getDecomposer().get()->getDecomposeQueue();
    
    FunctionalQueue temp_login;
    if (!(temp_login.init(dcr::g_getLoginQueue().get(), FQ_LOGIN_TEMP))) {
        LOG(ERROR) << "init temp_login error";
        return;
    }
    
    FunctionalQueue temp_task;
    if (!(temp_task.init(dcr::g_getComputationTaskQueue().get(), FQ_TASK_TEMP))) {
        LOG(ERROR) << "init temp_task error";
        return;
    }
    
    FunctionalQueue temp_result;
    if (!(temp_result.init(dcr::g_getResultQueue().get(), FQ_RESULT_TEMP))) {
        LOG(ERROR) << "init temp_result error";
        return;
    }
    
    Dispatcher::m_lock.init();
    
    GroupInfo *info = group_in_table->getInfo();
    int32 redispatch_cycle = 0;
    while (true) {
        
        int32 move_len = login->moveToTemp(&temp_login, login->getLen());
        if (move_len != -1) {
            info->m_new_add_node_num = dcr::g_getStrategy()->addComputeNode(&temp_login, group_in_table);
        }
        temp_login.tempReleaseToGlobalFree();
        
        
        move_len = result->moveToTemp(&temp_result, result->getLen());
        info->m_processed_result_num = 0;
        if (move_len != -1 && move_len != 0) {
            info->m_processed_result_num = dcr::g_getStrategy()->processResult(&temp_result, group_in_table);
        }
        temp_result.tempReleaseToGlobalFree();
        group_task_free->tempReleaseToGlobalFree();
        
        
        if (redispatch_cycle < dcr::g_getConfig()->getRedispatchTime()) {
            redispatch_cycle ++;
        } else {
            redispatch_cycle = 0;
            info->m_redispatch_num_one_cycle = dcr::g_getStrategy()->redispatchTask(decompose_task, group_in_table);
            info->m_redispatch_num_all_cycles += info->m_redispatch_num_one_cycle;
            info->m_redispatch_num_one_cycle = 0;
            
        }
        
        if (redispatch_cycle == dcr::g_getConfig()->getRedispatchTime() - 1) {
            if (checkFinish(group_in_table)) {
                int32 node_index = active_compute_node->getHead();
                ComputeNodeInfo *node = NULL;
                while (node_index != -1) {
                    node = static_cast<ComputeNodeInfo*>(active_compute_node->getMemoryBody(node_index));
                    UdpEndWork end;
                    end.endWork = true;
                    dcr::g_getSchedulerCommunicator()->udpEndWork(node->ip, &end);
                    int32 next = active_compute_node->getFqNextByIndex(node_index);
                    node_index = next;
                }
                Dispatcher::m_lock.Lock();
                LOG(INFO) << " group " << group_seq << " exit! " << endl;
                Dispatcher::m_lock.unLock();
                return;
            }
        }
        
        
        
        int32 node_index = active_compute_node->getHead();
        info->m_send_task_num = 0;
        info->m_load_task_num = 0;
        info->m_disconnect_node_per_cycle = 0;
        vector<string> delete_ip_list;
        bool delete_flag = false;
        while (node_index != -1) {
            ComputeNodeInfo *node = (ComputeNodeInfo*)active_compute_node->getMemoryBody(node_index);
            if (dcr::g_getStrategy()->checkDisconnect(node)) {
                info->m_disconnect_node_per_cycle++;
                char delete_ip[16];
                memcpy(delete_ip, node->ip, 16);
                delete_ip_list.push_back(delete_ip);
                delete_flag = true;
                dcr::g_getStrategy()->deleteComputeNode(node_index, decompose_task, group_in_table);
            } else {
                info->m_load_task_num += dcr::g_getStrategy()->loadTask(node, decompose_task, group_seq);
                info->m_send_task_num += dcr::g_getStrategy()->sendTask(node, node_index, group_in_table);
            }
            node_index = active_compute_node->getFqNextByIndex(node_index);
        }
        if (delete_flag) {
            dcr::g_getXMLLog()->add_DeleteComputeNode(delete_ip_list);
            LOG(INFO) << " delete compute node";
            delete_ip_list.clear();
        }

        info->m_send_task_num_all += info->m_send_task_num;
        info->m_load_task_num_all += info->m_load_task_num;
        info->m_disconnect_node_num += info->m_disconnect_node_per_cycle;
        info->m_active_node_num = active_compute_node->getLen();
        
        Dispatcher::m_lock.Lock();
        cout << "--------------------------------- Group " << group_seq << " Info -----------------------------------"<<endl;
        cout << "Acitve_cn:\t"<<info->m_active_node_num << "\tAdded_cn:\t"<< info->m_new_add_node_num<<"\tDisconnent_all: \t"
        << info->m_disconnect_node_num << "\tResult_packet:\t" << info->m_processed_result_num << endl;
        
        cout << "Load_task:\t" << info->m_load_task_num << "\tSend_task:\t" << info->m_send_task_num << "\tRedispatch_one:\t"
        << info->m_redispatch_num_one_cycle  << "\tDisconnect_one\t" << info->m_disconnect_node_per_cycle <<endl;
        
        cout << "Decompser queue len: \t" << dcr::g_getDecomposer()->getDecomposeQueue()->getLen() << "\tload_task_all\t"
        << info->m_load_task_num_all << " \tSend_task_all\t " << info->m_send_task_num_all << "\tTotal_redisp:\t"<<info->m_redispatch_num_all_cycles<< endl;
        cout <<"---------------------------------------------------------------------------------------" << endl;
        Dispatcher::m_lock.unLock();
        
        sleep(dcr::g_getConfig()->getSchedulerTime());
    }
}

int32 Strategy::addComputeNode(FunctionalQueue *login_list, GroupInTable* group) {
    CHECK_NOTNULL(login_list);
    
    LoginPacket *login_packet;
    time_t now_time = time(NULL);
    int32 login_index = login_list->getHead();
    int32 add_num = 0;
    vector<string> ip_list;
    while (login_index != -1) {
        login_packet = (LoginPacket*)login_list->getMemoryBody(login_index);
        char login_ip[16];
        memcpy(login_ip, login_packet->client_ip, 16);
        ip_list.push_back(login_ip);
        if (addOneNode(login_packet, now_time, group)) {
            add_num++;
        }
        int32 next = login_list->getFqNextByIndex(login_index);
        login_index = next;
    }
    if (ip_list.size() > 0) {
        dcr::g_getXMLLog()->add_ComputeNode(ip_list);
        ip_list.clear();
    }
    return add_num;
}

bool Strategy::addOneNode(LoginPacket *login_packet, time_t now, GroupInTable* group) {
    CHECK_NOTNULL(login_packet);
    
    unordered_set<unsigned long>::iterator find_end = dcr::g_getComputeNodeTable()->active_ip_list.end();
    
    if(dcr::g_getComputeNodeTable()->active_ip_list.find(inet_addr(login_packet->client_ip)) != find_end) {
        login_packet->client_socket->tcpClose();
        delete login_packet->client_socket;
        LOG(INFO) << " Find node in active_ip_list ";
        return false;
    } else {
        FunctionalQueue temp_node_queue;
        temp_node_queue.init(group->m_compute_node_queue, FQ_ACTIVE_CN_TEMP);
        int32 ret = temp_node_queue.tempAllocFromGlobalFree(1);
        if (ret == 0) {
            LOG(ERROR) << " init temp node queue failed ";
            temp_node_queue.tempReleaseToGlobalFree();
            return false;
        }
        int32 cn_index = temp_node_queue.getHead();
        LoginAck login_ack;
        login_ack.cn_index = cn_index;
        login_ack.group_index = group->m_group_seq;
        ret = dcr::g_getSchedulerCommunicator()->tcpAckSend(login_packet->client_socket, &login_ack);
        if (!ret) {
            LOG(ERROR) << " send ack failed ";
            temp_node_queue.tempReleaseToGlobalFree();
            return false;
        }
        login_packet->client_socket->tcpClose();
        delete login_packet->client_socket;
        
        ComputeNodeInfo *node_info = (ComputeNodeInfo*)temp_node_queue.getMemoryBody(cn_index);
        node_info->init(login_packet->client_ip, now, 0);
        group->m_active_compute_node->moveFromTemp(&temp_node_queue, temp_node_queue.getLen());
        
        dcr::g_getComputeNodeTable()->active_ip_list.insert(inet_addr(login_packet->client_ip));
    }
    return true;
}

int32 Strategy::deleteComputeNode(int32 delete_index, FunctionalQueue *decomposer_queue, GroupInTable* group) {
    ComputeNodeInfo *node = (ComputeNodeInfo*)group->m_active_compute_node->getMemoryBody(delete_index);
    unordered_set<unsigned long>::iterator find_end = dcr::g_getComputeNodeTable()->active_ip_list.end();
    
    if (dcr::g_getComputeNodeTable()->active_ip_list.find(inet_addr(node->ip)) != find_end) {
        
        int32 len = node->m_task_wait_to_send.getLen();
        node->m_task_wait_to_send.moveToTemp(decomposer_queue, len);
        len = node->m_task_already_send.getLen();
        node->m_task_already_send.moveToTemp(decomposer_queue, len);
        
        FunctionalQueue *global_free = group->m_compute_node_queue->getGlobalFree();
        group->m_active_compute_node->moveOneElementTo(global_free, delete_index);
        dcr::g_getComputeNodeTable()->active_ip_list.erase(inet_addr(node->ip));
        return 1;
    } else {
        return 0;
    }
}

int32 Strategy::redispatchTask(FunctionalQueue *decompose_queue, GroupInTable* group) {
    CHECK_NOTNULL(decompose_queue);
    int32 redisp_all_node = 0;
    int32 redisp_index = group->m_active_compute_node->getHead();
    time_t now = time(NULL);
    ComputeNodeInfo* node = NULL;
    
    while (redisp_index != -1) {
        int32 redisp_one_node = 0;
        node = (ComputeNodeInfo*)group->m_active_compute_node->getMemoryBody(redisp_index);
        
        if (redispatchOneNode(node, decompose_queue, now, redisp_one_node, group)) {
            redisp_all_node += redisp_one_node;
        }
        int32 next = group->m_active_compute_node->getFqNextByIndex(redisp_index);
        redisp_index = next;
    }
    return redisp_all_node;
}

//调试数据
//int dispatch_index = 1000;

bool Strategy::redispatchOneNode(ComputeNodeInfo *node, FunctionalQueue *decompose_queue,
                                 time_t now, int32 &redisp_one_node, GroupInTable* group) {
    CHECK_NOTNULL(node);
    int32 already_send_index = node->m_task_already_send.getHead();
   
    unit_index_t *unit_index = NULL;
    while (already_send_index != -1) {
        unit_index = node->m_task_already_send.getOneUnitIndex(already_send_index);
        int next = node->m_task_already_send.getFqNextByIndex(already_send_index);
        if (difftime(now, unit_index->time) > dcr::g_getConfig()->getTaskLostTime()) {
            UdpSendTask *udp_task = (UdpSendTask*)node->m_task_already_send.getMemoryBody(already_send_index);
            Dispatcher::m_lock.Lock();
            LOG(WARNING) << " Redispatch Task: IP: " << node->ip << " group_index: " << group->m_group_seq
            << " node_index: " << udp_task->task_head.compute_node_index
            << " task_index: " << udp_task->task_head.mq_task_index << " task_id:" << udp_task->task_head.task_ID
			<< " original task_id: " << udp_task->task_head.original_task_ID;
            Dispatcher::m_lock.unLock();
            
            node->m_mutex.Lock();
			
			//如果重发包，就需要将其ID换成新的不重复的ID，以免接收到旧的结果包。
			udp_task->task_head.task_ID = redispatch_index ++;
			//LOG(WARNING) << " assign new task id " << udp_task->task_head.task_ID;
			
            int err = node->m_task_already_send.moveOneElementTo(decompose_queue, already_send_index);
            if (err == 0) {
                LOG(ERROR) << " error index: " << already_send_index;
            }
            node->m_mutex.unLock();
            redisp_one_node++;
        } else {
            UdpSendTask *udp_task = (UdpSendTask*)node->m_task_already_send.getMemoryBody(already_send_index);
            /*
            Dispatcher::m_lock.Lock();
            LOG(WARNING) << " task id: " << udp_task->task_head.task_ID << " orginal_task id: "
            << udp_task->task_head.original_task_ID << " diff time: " << difftime(now, unit_index->time)
            << " lost time: " << dcr::g_getConfig()->getTaskLostTime();
            Dispatcher::m_lock.unLock();
             */
            break;
        }
        already_send_index = next;
    }
    return true;
}

int32 Strategy::computeDispatchNum(int32 wait_queue_len) {
    int32 ret_val = 0;
    int32 queue_len = dcr::g_getConfig()->getTaskQueuePerNode();
    
    if (wait_queue_len < (2*queue_len/3)) {
        ret_val = (2*queue_len/3) - wait_queue_len;
    }
    return ret_val;
}

int32 Strategy::loadTask(ComputeNodeInfo *node, FunctionalQueue *decompose_queue, int group_seq) {
    CHECK_NOTNULL(node);
    CHECK_NOTNULL(decompose_queue);
    int32 wait_queue_len = node->m_task_wait_to_send.getLen();
    int32 dispatch_num = this->computeDispatchNum(wait_queue_len);
    
    FunctionalQueue temp_load;
    if(!(temp_load.init(dcr::g_getComputationTaskQueue().get(), FQ_TASK_TEMP))) {
        LOG(ERROR) << " temp_load init failed: ";
        return 0;
    }
    int32 move_len = decompose_queue->moveToTemp(&temp_load, dispatch_num);
    if (move_len == -1) {
        return 0;
    }
    move_len = node->m_task_wait_to_send.moveFromTemp(&temp_load, move_len);
    if (move_len == -1) {
        return 0;
    }
    return move_len;
}

int32 Strategy::computeSendNum(int32 cq_current) {
    int32 ret_val = 0;
    int32 queue_len = dcr::g_getConfig()->getTaskQueuePerNode();
    //保持计算几点三分之一满的状态
    if (cq_current < (2 * queue_len/3)) {
        ret_val = (2*queue_len/3 - cq_current);
    }
    return ret_val;
}

int32 Strategy::sendTask(ComputeNodeInfo *node, int32 node_index, GroupInTable* group) {
    CHECK_NOTNULL(node);
    int32 cn_task_len = node->m_cq_task_len;
    int32 send_num = this->computeSendNum(cn_task_len);
    
    int32 send_index = node->m_task_wait_to_send.getHead();
    int32 actual_num = 0;
    time_t now = time(NULL);
    node->m_mutex.Lock();
    while ((actual_num < send_num) && send_index != -1) {
        UdpSendTask *udp_task = (UdpSendTask*)node->m_task_wait_to_send.getMemoryBody(send_index);
        
        udp_task->task_head.group_seq = group->m_group_seq;
        udp_task->task_head.mq_task_index = send_index;
        udp_task->task_head.compute_node_index = node_index;
        int32 original_ID = udp_task->task_head.original_task_ID;
        //LOG(ERROR) << " original id is " << original_ID << " and send num is " 
        //<< dcr::g_getTaskManager()->getTaskByID(original_ID)->send_packet_num;
        if (dcr::g_getSchedulerCommunicator()->udpComputationTaskSend(node->ip, udp_task)) {
            actual_num++;
            cn_task_len++;
            //注意此处是否会发生死锁
            Task::m_lock.Lock();
            dcr::g_getTaskManager()->getTaskByID(original_ID)->send_packet_num ++;
            Task::m_lock.unLock();
        } else {
            continue;
        }
        int32 next = node->m_task_wait_to_send.getFqNextByIndex(send_index);
        node->m_task_wait_to_send.getOneUnitIndex(send_index)->time = now;
        int err = node->m_task_wait_to_send.moveOneElementTo(&(node->m_task_already_send), send_index);
		if(err == 0) {
			LOG(ERROR) << " error index: " << send_index;
		}
        send_index = next;
    }
    node->m_cq_task_len = cn_task_len;
    node->m_mutex.unLock();
    return actual_num;
}

bool Strategy::checkDisconnect(ComputeNodeInfo *node) {
    CHECK_NOTNULL(node);
    time_t now = time(NULL);
    if (difftime(now, node->m_last_access) > dcr::g_getConfig()->getDisconnetTime()) {
        LOG(INFO) << node->ip <<" disconnect! ";
        return true;
    } else {
        return false;
    }
}

int32 Strategy::processResult(FunctionalQueue *result_list, GroupInTable* group) {
    CHECK_NOTNULL(result_list);
    SchedUdpRecv *result = NULL;
    int32 process_num = 0;
    int32 result_index = result_list->getHead();
    time_t now = time(NULL);
    while (result_index != -1) {
        result = (SchedUdpRecv*)result_list->getMemoryBody(result_index);
        int32 next = result_list->getFqNextByIndex(result_index);
        if(this->processOneResult(result, now, group)) {
            process_num++;
        }
        result_index = next;
    }
    return process_num;
}

bool Strategy::processOneResult(SchedUdpRecv *result, time_t now, GroupInTable* group) {
    CHECK_NOTNULL(result);
    switch (result->compute_result.packet_type) {
        case COMPUTE_RESULT: {
            int32 cn_index = result->compute_result.head.compute_node_index;
            if(!group->m_active_compute_node->checkIndex(cn_index)) {
                LOG(ERROR) << " Check error! cn_index: " << cn_index;
                return false;
            }
            ComputeNodeInfo *cn = (ComputeNodeInfo*)group->getActiveComputeNode()->getMemoryBody(cn_index);
            int task_index = result->compute_result.head.mq_task_index;
            unit_index_t *unit = cn->m_task_already_send.getOneUnitIndex(task_index);
            //动态调整 redispatch 的周期
            
            if (dcr::g_getConfig()->getTaskLostTime() < difftime(now, unit->time) + 150) {
                  int lost_time = difftime(now, unit->time) + 150;
                  dcr::g_getConfig()->updateTaskLostTime(lost_time);
				  LOG(WARNING) << " difftime is " << difftime(now, unit->time) << " and update dispatch time is " << lost_time;
            }
            
            
            if (!cn->m_task_already_send.checkIndex(task_index)) {
                LOG(ERROR) << " Check error! task index: " << task_index;
                return false;
            }
            
            UdpSendTask *udp_task = (UdpSendTask*)cn->m_task_already_send.getMemoryBody(task_index);
            int32 task_ID = result->compute_result.head.task_ID;
            if (udp_task->task_head.task_ID != task_ID) {
                LOG(ERROR) << " Check error! task_ID: " << task_ID;
                return false;
            }
            
			
            char cn_ip[16];
            memcpy(cn_ip, result->compute_result.head.cn_ip, 16);
            
            if(strcmp(cn->ip, cn_ip) != 0) {
                LOG(ERROR) << " Check error! ";
                return false;
            }
            
            if (udp_task->task_head.original_task_ID != result->compute_result.head.original_task_ID) {
                LOG(ERROR) << " Check error! ";
                return false;
            }
            
			
            cn->m_mutex.Lock();
            int err = cn->m_task_already_send.moveOneElementTo(group->m_group_free, task_index);
            if (err == 0) {
                LOG(ERROR) << " error index: " << task_index;
            }
            
            cn->m_last_access = now;
            cn->m_cq_task_len = result->compute_result.head.task_queue_len;
            cn->m_mutex.unLock();
            //对结果进行归约， 要按original_task的编号。
            int32 original_task_ID = result->compute_result.head.original_task_ID;
            Result *reduce_result = dcr::g_getDecomposer()->getFinalResultByID(original_task_ID);
            int32 offset = sizeof(UdpComputeResultHead) + sizeof(SchedUdpRecvType);
            
            Result *compute_result = (Result*)((char*)result + offset);
            Dispatcher::m_lock.Lock();
            //对结果进行规约
            dcr::g_getJob()->reduce(compute_result, reduce_result);
            //统计回收的结果数量
            dcr::g_getDecomposer()->m_result_num[original_task_ID]++;

            Dispatcher::m_lock.unLock();

            Task::m_lock.Lock();
            Task *result_task = dcr::g_getTaskManager()->getTaskByID(original_task_ID);
            result_task->finish_packet_num ++;
            //判断任务是否完成，如果完成则写入LOG完成任务
            int32 finish_packet_num = result_task->finish_packet_num;
            int32 send_packet_num = result_task->send_packet_num;
            int32 redispatch_packet_num = result_task->redispatch_packet_num;
            int32 decompose_num = result_task->decompose_packet_num;
            //LOG(WARNING) << " in process result decompose_num is " << decompose_num;
            //此处应该是finish_packet_num == decompose_num
            if (send_packet_num == finish_packet_num + redispatch_packet_num && finish_packet_num == decompose_num) {
                dcr::g_getXMLLog()->add_finishJob(*result_task);
                result_task->state = FINISH;
            } 
            Task::m_lock.unLock();

            string result_path = dcr::g_getDecomposer()->m_id_resultName[original_task_ID];
            OriginalTask *originalTask = dcr::g_getDecomposer()->getOriginalTaskByID(original_task_ID);
            dcr::g_getJob()->updateResultFile(originalTask, original_task_ID, dcr::g_getDecomposer().get(), compute_result, reduce_result, result_path);

            break;
        }
        case HEART_BEAT: {
            int32 cn_index = result->heartbeat.compute_node_index;
            bool ret_val_1 = group->m_active_compute_node->checkIndex(cn_index);
            char cn_ip[16];
            memcpy(cn_ip, result->heartbeat.cn_ip, 16);

            ComputeNodeInfo *cn = (ComputeNodeInfo*)group->getActiveComputeNode()->getMemoryBody(cn_index);
            bool ret_val_2 = (strcmp(cn_ip, cn->ip) == 0 ? true : false);
                
            if(ret_val_1 && ret_val_2){
                cn->m_mutex.Lock();
                cn->m_last_access = now;
                cn->m_cq_task_len = result->heartbeat.task_queue_len;
                cn->m_mutex.unLock();
            } else {
                LOG(ERROR) << " cn_index error " << " group_seq : " << result->heartbeat.group_seq
                << " cn index : "<< result->heartbeat.compute_node_index;
                return false;
            }
            
            break;
        }
        default: {
            LOG(ERROR) << " unkown result packet type ";
            return false;
        }
    }
    return true;
}

bool Strategy::checkFinish(GroupInTable* group) {
    CHECK_NOTNULL(group);
    
	bool finished = true;
	size_t task_num = dcr::g_getDecomposer()->m_decompose_num.size();
    if (task_num == 0) {
        return false;
    }
    for (int32 i = 1; i <= task_num; i++) {
        
        if (dcr::g_getDecomposer()->m_decompose_num[i] == 0) {
            finished = false;
        }
        
        if (dcr::g_getDecomposer()->m_decompose_num[i] != dcr::g_getDecomposer()->m_result_num[i]) {
            Dispatcher::m_lock.Lock();
            //LOG(INFO) << " task: " << i << " decompose_num: " << dcr::g_getDecomposer()->m_decompose_num[i]
            //<< " result_num: " << dcr::g_getDecomposer()->m_result_num[i];
            finished = false;
            Dispatcher::m_lock.unLock();
        }
    }
    return false;
}

bool StrategyBlast::init() {
    this->m_mutex = new Mutex();
    this->m_mutex->init();
    this->m_type_history = new map<unsigned long, unordered_set<int32>*>[dcr::g_getConfig()->getNodeGroup()];
    return true;
}

bool StrategyBlast::destroy() {
    delete m_mutex;
    
    for (int32 i = 0; i < dcr::g_getConfig()->getNodeGroup(); i++) {
        map<unsigned long, unordered_set<int32>*>::iterator it = m_type_history[i].begin();
        for (; it !=  m_type_history[i].end(); it++) {
            delete it->second;
            m_type_history[i].erase(it);
        }
    }
    delete []m_type_history;
    return true;
}

void StrategyBlast::decomposeStrategy(Decomposer* ptr) {
    int32 task_ID = 0;
    FunctionalQueue temp_decomposer_queue;
    temp_decomposer_queue.init(dcr::g_getComputationTaskQueue().get(), FQ_DECOMPOSER_TEMP);
    map<int32, OriginalTask*>::iterator task_it = ptr->m_original_task_queue.begin();
    map<int32, Granularity*>::iterator granulity_it = ptr->m_granulity_queue.begin();
    scoped_ptr<ComputationTask> new_task;
    
    unordered_multimap<int32, int32> temp_type_index;
    
    int32 original_type = 0;
    int32 compute_type = 0;
    for (; task_it != ptr->m_original_task_queue.end(); task_it++, granulity_it++) {
        
        //获取原始任务类型
        original_type = task_it->second->getType();
        //计算任务类型
        compute_type = 1;
        
        int32 computeSpeed = ptr->computeDecomposeSpeed();
        temp_decomposer_queue.tempAllocFromGlobalFree(computeSpeed);
        int32 temp_queue_index = temp_decomposer_queue.getHead();
        
        //为了按照一个字节进行偏移采用的办法
        UdpSendTask *body = reinterpret_cast<UdpSendTask*>(temp_decomposer_queue.getMemoryBody(temp_queue_index));
        
        //第一次因为要进行初始化所以要独立开来
        new_task.reset(ptr->getTask(task_it->first, task_it->second, granulity_it->second, true));
        
        int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
        memcpy(((char*)body + offset), new_task.get(), dcr::g_getComputeTaskSize());
        
        temp_type_index.insert(make_pair(original_type*10000 + compute_type, temp_queue_index));
        
        
        body->task_head.original_task_ID = task_it->first;
        int32 already_decompose = 1;
        
        int32 next_index = temp_decomposer_queue.getFqNextByIndex(temp_queue_index);
        temp_queue_index = next_index;
        
        while (!(ptr->isFinishGetTask())) {
            if (already_decompose >= computeSpeed) {
                ptr->m_decompose_queue->moveFromTemp(&temp_decomposer_queue,
                                                     temp_decomposer_queue.getLen());
                temp_decomposer_queue.tempReleaseToGlobalFree();
                ptr->m_decompose_num[task_it->first] += already_decompose;
                already_decompose = 0;
                computeSpeed = ptr->computeDecomposeSpeed();
                temp_decomposer_queue.tempAllocFromGlobalFree(computeSpeed);
                temp_queue_index = temp_decomposer_queue.getHead();
                
                unordered_multimap<int, int>::iterator it = temp_type_index.begin();
                this->m_mutex->Lock();
                while (it != temp_type_index.end()) {
                    this->m_type_index.insert(*it);
                    it++;
                }
                this->m_mutex->unLock();
                
                temp_type_index.clear();
                sleep(dcr::g_getConfig()->getSchedulerTime());
            }
            
            body = static_cast<UdpSendTask*>(temp_decomposer_queue.getMemoryBody(temp_queue_index));
            new_task.reset(ptr->getTask(task_it->first, task_it->second, granulity_it->second, false));
            memcpy(((char*)body + offset), new_task.get(), dcr::g_getComputeTaskSize());
            body->task_head.original_task_ID = task_it->first;
            body->task_head.task_ID = task_ID++;
            
            already_decompose++;
            
            //将任务类型和对应的mq_index 下标放入
            compute_type = compute_type + 1;
            //LOG(WARNING) << " type : " << original_type*10000 + compute_type << " temp_queue_index: " << temp_queue_index;
            temp_type_index.insert(make_pair(original_type*10000 + compute_type, temp_queue_index));
            
            next_index = temp_decomposer_queue.getFqNextByIndex(temp_queue_index);
            temp_queue_index = next_index;
        }
        ptr->m_decompose_num[task_it->first] += already_decompose;
        //循环结束， 把最后一次的分解任务进行处理, 并将分解了多少个任务的信息保存起来
        ptr->m_decompose_queue->moveFromTemp(&temp_decomposer_queue,
                                             already_decompose);
        
        temp_decomposer_queue.tempReleaseToGlobalFree();
    }
}

void StrategyBlast::dispachStrategy(int group_seq) {
    GroupInTable *group_in_table = dcr::g_getComputeNodeTable()->getGroup(group_seq);
    FunctionalQueue *active_compute_node = group_in_table->getActiveComputeNode();
    FunctionalQueue *group_task_free = group_in_table->getGroupFree();
    FunctionalQueue *login = group_in_table->getLogin();
    FunctionalQueue *result = group_in_table->getResult();
    FunctionalQueue *decompose_task = dcr::g_getDecomposer().get()->getDecomposeQueue();
    
    FunctionalQueue temp_login;
    if (!(temp_login.init(dcr::g_getLoginQueue().get(), FQ_LOGIN_TEMP))) {
        LOG(ERROR) << "init temp_login error";
        return;
    }
    
    FunctionalQueue temp_task;
    if (!(temp_task.init(dcr::g_getComputationTaskQueue().get(), FQ_TASK_TEMP))) {
        LOG(ERROR) << "init temp_task error";
        return;
    }
    
    FunctionalQueue temp_result;
    if (!(temp_result.init(dcr::g_getResultQueue().get(), FQ_RESULT_TEMP))) {
        LOG(ERROR) << "init temp_result error";
        return;
    }
    
    Dispatcher::m_lock.init();
    
    GroupInfo *info = group_in_table->getInfo();
    int32 redispatch_cycle = 0;
    while (true) {
        
        int32 move_len = login->moveToTemp(&temp_login, login->getLen());
        if (move_len != -1) {
            info->m_new_add_node_num = dcr::g_getStrategy()->addComputeNode(&temp_login, group_in_table);
        }
        temp_login.tempReleaseToGlobalFree();
        
        
        move_len = result->moveToTemp(&temp_result, result->getLen());
        info->m_processed_result_num = 0;
        if (move_len != -1 && move_len != 0) {
            info->m_processed_result_num = dcr::g_getStrategy()->processResult(&temp_result, group_in_table);
        }
        temp_result.tempReleaseToGlobalFree();
        group_task_free->tempReleaseToGlobalFree();
        
        
        if (redispatch_cycle < dcr::g_getConfig()->getRedispatchTime()) {
            redispatch_cycle ++;
        } else {
            redispatch_cycle = 0;
            info->m_redispatch_num_one_cycle = dcr::g_getStrategy()->redispatchTask(decompose_task, group_in_table);
            info->m_redispatch_num_all_cycles += info->m_redispatch_num_one_cycle;
            info->m_redispatch_num_one_cycle = 0;
            
        }
        
        if (redispatch_cycle == dcr::g_getConfig()->getRedispatchTime() - 1) {
            if (checkFinish(group_in_table)) {
                int32 node_index = active_compute_node->getHead();
                ComputeNodeInfo *node = NULL;
                while (node_index != -1) {
                    node = static_cast<ComputeNodeInfo*>(active_compute_node->getMemoryBody(node_index));
                    UdpEndWork end;
                    end.endWork = true;
                    dcr::g_getSchedulerCommunicator()->udpEndWork(node->ip, &end);
                    int32 next = active_compute_node->getFqNextByIndex(node_index);
                    node_index = next;
                }
                Dispatcher::m_lock.Lock();
                LOG(INFO) << " group " << group_seq << " exit! " << endl;
                Dispatcher::m_lock.unLock();
                return;
            }
        }
        
        int32 node_index = active_compute_node->getHead();
        info->m_send_task_num = 0;
        info->m_load_task_num = 0;
        info->m_disconnect_node_per_cycle = 0;
        while (node_index != -1) {
            ComputeNodeInfo *node = (ComputeNodeInfo*)active_compute_node->getMemoryBody(node_index);
            if (dcr::g_getStrategy()->checkDisconnect(node)) {
                info->m_disconnect_node_per_cycle++;
                dcr::g_getStrategy()->deleteComputeNode(node_index, decompose_task, group_in_table);
            } else {
                //注意此处的加锁，与普通的调度算法区别
                this->m_mutex->Lock();
                info->m_load_task_num += dcr::g_getStrategy()->loadTask(node, decompose_task, group_seq);
                this->m_mutex->unLock();
                info->m_send_task_num += dcr::g_getStrategy()->sendTask(node, node_index, group_in_table);
            }
            node_index = active_compute_node->getFqNextByIndex(node_index);
        }
        info->m_send_task_num_all += info->m_send_task_num;
        info->m_load_task_num_all += info->m_load_task_num;
        info->m_disconnect_node_num += info->m_disconnect_node_per_cycle;
        info->m_active_node_num = active_compute_node->getLen();
        
        Dispatcher::m_lock.Lock();
        cout << "--------------------------------- Group " << group_seq << " Info -----------------------------------"<<endl;
        cout << "Acitve_cn:\t"<<info->m_active_node_num << "\tAdded_cn:\t"<< info->m_new_add_node_num<<"\tDisconnent_all: \t"
        << info->m_disconnect_node_num << "\tResult_packet:\t" << info->m_processed_result_num << endl;
        
        cout << "Load_task:\t" << info->m_load_task_num << "\tSend_task:\t" << info->m_send_task_num << "\tRedispatch_one:\t"
        << info->m_redispatch_num_one_cycle  << "\tDisconnect_one\t" << info->m_disconnect_node_per_cycle <<endl;
        
        cout << "Decompser queue len: \t" << dcr::g_getDecomposer()->getDecomposeQueue()->getLen() << "\tload_task_all\t"
        << info->m_load_task_num_all << " \tSend_task_all\t " << info->m_send_task_num_all << "\tTotal_redisp:\t"<<info->m_redispatch_num_all_cycles<< endl;
        cout <<"---------------------------------------------------------------------------------------" << endl;
        Dispatcher::m_lock.unLock();
        
        sleep(dcr::g_getConfig()->getSchedulerTime());
    }
}



int32 StrategyBlast::loadTask(ComputeNodeInfo *node, FunctionalQueue *decompose_queue, int32 group_seq) {
    CHECK_NOTNULL(node);
    CHECK_NOTNULL(decompose_queue);
    int32 wait_queue_len = node->m_task_wait_to_send.getLen();
    int32 dispatch_num = this->computeDispatchNum(wait_queue_len);
    int32 ret_val = 0;
    
    FunctionalQueue temp_load;
    if(!(temp_load.init(dcr::g_getComputationTaskQueue().get(), FQ_TASK_TEMP))) {
        LOG(ERROR) << " temp_load init failed: ";
        return 0;
    }
    
    LOG(WARNING) << " node ip: " << node->ip;
    unsigned long ip = inet_addr(node->ip);
    if (m_type_history[group_seq].find(ip) == m_type_history[group_seq].end()) {
        m_type_history[group_seq].insert(pair<unsigned long, unordered_set<int32>*>(ip, new unordered_set<int32>()));
    }
    
    
    //取出这个node的type
    unordered_set<int32>::iterator history_it = m_type_history[group_seq][ip]->begin();
    
    for ( ; ret_val < dispatch_num && m_type_index.size() != 0; ret_val++) {
        
        if (history_it == m_type_history[group_seq][ip]->end()) {
            unordered_multimap<int32, int32>::iterator index_it = m_type_index.begin();
            
            size_t div = 1;
            if (m_type_index.size() > 1) {
                div = m_type_index.size();
            }
            int32 rand_pos = rand() % (div);
            
            //在m_type_index 中进行随机选择
            while (rand_pos > 0) {
                index_it++;
                rand_pos--;
            }
            
            LOG(WARNING) << " type: " << index_it->first << " counting: " << m_type_index.count(index_it->first);
            node->m_mutex.Lock();
            int err = decompose_queue->moveOneElementTo(&(node->m_task_wait_to_send), index_it->second);
            node->m_mutex.unLock();
			if(err == 0) {
				LOG(ERROR) << " error index: " << index_it->second;
				ret_val--;
				break;
			}
            m_type_history[group_seq][ip]->insert(index_it->first);
            history_it = m_type_history[group_seq][ip]->begin();
            m_type_index.erase(index_it);
            
        } else {
            
            int32 type = *history_it;
            size_t count = m_type_index.count(type);
            //现在任务中没有该节点历史的任务类型
            while (count == 0) {
                
                
                history_it++;
                if (history_it == m_type_history[group_seq][ip]->end()) {
                    break;
                }
                
                type = *history_it;
                count = m_type_index.count(type);
            }
            
            if (history_it == m_type_history[group_seq][ip]->end()) {
                ret_val--;
                break;
            }
            
            unordered_map<int32, int32>::iterator find_it = m_type_index.find(type);
            LOG(WARNING) << " type: " << type << " counting: " << count;
            node->m_mutex.Lock();
            int err = decompose_queue->moveOneElementTo(&(node->m_task_wait_to_send), find_it->second);
            node->m_mutex.unLock();
			if(err == 0) {
				LOG(ERROR) << " error index: " << find_it->second;
				ret_val--;
				continue;
			}
            m_type_index.erase(find_it);
            
            if (m_type_index.count(type) == 0) {
                m_type_history[group_seq][ip]->erase(find_it->first);
                m_type_history[group_seq][ip]->insert(find_it->first);
                history_it = m_type_history[group_seq][ip]->begin();
            }
            
        }
        
    }
    
    if(m_type_index.size() == 0 && decompose_queue->getLen() != 0) {
        LOG(WARNING) << " move redispatch queue len: " << decompose_queue->getLen();
        decompose_queue->moveToTemp(&(node->m_task_wait_to_send), decompose_queue->getLen());
        ret_val += decompose_queue->getLen();
    }
    
    return ret_val;
}

bool StrategyType::init() {
    Strategy::init();
    this->m_mutex = new Mutex();
    this->m_mutex->init();
    //this->m_ip_type = new map<unsigned long, unordered_set<int32>*>;
    //读取map_resource.xml文件，填写节点到任务类型的映射表。
    initNodeTypeMap();
}

bool StrategyType::initNodeTypeMap(){
    string node_type_file_name = dcr::g_getNodeTypeFileName();
    //TiXmlDocument *p_doc = new TiXmlDocument(node_type_file_name.c_str());
    TiXmlDocument p_doc(node_type_file_name.c_str());
    if (!(p_doc.LoadFile())) {
        LOG(ERROR) << " Load node_type file failed ";
        return false;
    }
    
    TiXmlElement *root_element = p_doc.RootElement();
    TiXmlElement *node = root_element->FirstChildElement("Node");
    CHECK_NOTNULL(node);
    this->m_mutex->Lock();
    while(node){
        unordered_set<int32>* tempset = new  unordered_set<int32>;
        TiXmlElement *ipelem = node->FirstChildElement("IP");
        CHECK_NOTNULL(ipelem);
        string ipstr = ipelem->GetText();
        unsigned long ip = inet_addr(ipstr.c_str());
        TiXmlElement *typeele = node->FirstChildElement("Type");
        CHECK_NOTNULL(typeele);
        while(typeele) {
            string typestr = typeele->GetText();
            tempset->insert(atoi(typestr.c_str()));
            typeele = typeele->NextSiblingElement("Type");
        }
        this->m_ip_type.insert(make_pair(ip, tempset));
        node = node->NextSiblingElement("Node");
    }
    this->m_mutex->unLock();
}

bool StrategyType::destroy() {
    delete m_mutex;
    map<unsigned long, unordered_set<int32>*>::iterator it = m_ip_type.begin();
    for (; it !=  m_ip_type.end(); it++) {
        delete it->second;
        m_ip_type.erase(it);
    }
    return true;
    
}


void StrategyType::decomposeStrategy(Decomposer* ptr) {
    int32 task_ID = 0;
    FunctionalQueue temp_decomposer_queue;
    temp_decomposer_queue.init(dcr::g_getComputationTaskQueue().get(), FQ_DECOMPOSER_TEMP);
    scoped_ptr<ComputationTask> new_task;
    pair<int32, int32> type_prior_pair;
    unordered_multimap<int32, int32> temp_type_index;
    while (true) {

        Task *new_job_task = dcr::g_getTaskManager()->getNextTask();
        if (NULL == new_job_task) {
            sleep(5);
            continue;
        }
        int32 original_ID = ptr->decomposeNewTask(new_job_task);
        if (original_ID == -1) {
            LOG(ERROR) << " decompoes new task fail";
            return;
        }
        cout << "decompose succeed and id is " << original_ID << endl;

        int32 computeSpeed = ptr->computeDecomposeSpeed();
        temp_decomposer_queue.tempAllocFromGlobalFree(computeSpeed);
        int32 temp_queue_index = temp_decomposer_queue.getHead();
        
        UdpSendTask *body = reinterpret_cast<UdpSendTask*>(temp_decomposer_queue.getMemoryBody(temp_queue_index));

        string result_path = ptr->m_id_resultName[original_ID];
        OriginalTask *ori_task = ptr->getOriginalTaskByID(original_ID);
        Granularity *granu = ptr->getGranulityByID(original_ID);
        //更新结果文件
        dcr::g_getJob()->updateResultFileByDecomposer(ori_task, original_ID, granu, ptr, result_path, true);
        //ptr->updateResultFileByDecomposer(ori_task, original_ID, granu, result_path, true);
        //第一次因为要进行初始化所以要独立开来
        new_task.reset(ptr->getTask(original_ID, ori_task, granu, true));

        ComputationTask *ctask = new_task.get();
        type_prior_pair = ptr->getTypePrior(ori_task, granu, ctask, true);
        ctask->type = type_prior_pair.first;
        ctask->prior = type_prior_pair.second;

        temp_type_index.insert(make_pair(ctask->type, temp_queue_index));
        
        int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
        memcpy(((char*)body + offset), new_task.get(), dcr::g_getComputeTaskSize());
        
        body->task_head.original_task_ID = original_ID;
        body->task_head.task_ID = task_ID++;
        int32 already_decompose = 1;
        
        int32 next_index = temp_decomposer_queue.getFqNextByIndex(temp_queue_index);
        temp_queue_index = next_index;

        while (!(ptr->isFinishGetTask())) {
            if (already_decompose >= computeSpeed) {
                ptr->m_decompose_queue->moveFromTemp(&temp_decomposer_queue,
                                                     temp_decomposer_queue.getLen());
                temp_decomposer_queue.tempReleaseToGlobalFree();
                ptr->m_decompose_num[original_ID] += already_decompose;
                already_decompose = 0;
                computeSpeed = ptr->computeDecomposeSpeed();
                temp_decomposer_queue.tempAllocFromGlobalFree(computeSpeed);
                temp_queue_index = temp_decomposer_queue.getHead();
                sleep(dcr::g_getConfig()->getSchedulerTime());
            }

        
            dcr::g_getJob()->updateResultFileByDecomposer(ori_task, original_ID, granu, ptr, result_path, false);
            //ptr->updateResultFileByDecomposer(ori_task, original_ID, granu, result_path, false);
            body = reinterpret_cast<UdpSendTask*>(temp_decomposer_queue.getMemoryBody(temp_queue_index));
            new_task.reset(ptr->getTask(original_ID, ori_task, granu, false));

            ctask = new_task.get();
            type_prior_pair = ptr->getTypePrior(ori_task, granu, ctask, false);
            ctask->type = type_prior_pair.first;
            ctask->prior = type_prior_pair.second;
            temp_type_index.insert(make_pair(ctask->type, temp_queue_index));

            memcpy(((char*)body + offset), new_task.get(), dcr::g_getComputeTaskSize());
            body->task_head.original_task_ID = original_ID;
            body->task_head.task_ID = task_ID++;
            already_decompose++;
            
            next_index = temp_decomposer_queue.getFqNextByIndex(temp_queue_index);
            temp_queue_index = next_index;
        }

        this->m_mutex->Lock();
        unordered_multimap<int, int>::iterator it = temp_type_index.begin();
        while (it != temp_type_index.end()) {
            this->m_type_index.insert(*it);
            it++;
        }
        temp_type_index.clear();
        ptr->m_decompose_num[original_ID] += already_decompose;
        //循环结束， 把最后一次的分解任务进行处理, 并将分解了多少个任务的信息保存起来
        ptr->m_decompose_queue->moveFromTemp(&temp_decomposer_queue,
                                             already_decompose);
        temp_decomposer_queue.tempReleaseToGlobalFree();
        redispatch_index = task_ID;
        this->m_mutex->unLock();
        new_job_task->decompose_packet_num =  ptr->m_decompose_num[original_ID];
        //LOG(WARNING) << " decompoes num is " <<  new_job_task->decompose_packet_num;
    
    }

}

void StrategyType::dispachStrategy(int group_seq) {
    GroupInTable *group_in_table = dcr::g_getComputeNodeTable()->getGroup(group_seq);
    FunctionalQueue *active_compute_node = group_in_table->getActiveComputeNode();
    FunctionalQueue *group_task_free = group_in_table->getGroupFree();
    FunctionalQueue *login = group_in_table->getLogin();
    FunctionalQueue *result = group_in_table->getResult();
    FunctionalQueue *decompose_task = dcr::g_getDecomposer().get()->getDecomposeQueue();
    
    FunctionalQueue temp_login;
    if (!(temp_login.init(dcr::g_getLoginQueue().get(), FQ_LOGIN_TEMP))) {
        LOG(ERROR) << "init temp_login error";
        return;
    }
    
    FunctionalQueue temp_task;
    if (!(temp_task.init(dcr::g_getComputationTaskQueue().get(), FQ_TASK_TEMP))) {
        LOG(ERROR) << "init temp_task error";
        return;
    }
    
    FunctionalQueue temp_result;
    if (!(temp_result.init(dcr::g_getResultQueue().get(), FQ_RESULT_TEMP))) {
        LOG(ERROR) << "init temp_result error";
        return;
    }
    
    Dispatcher::m_lock.init();
    
    GroupInfo *info = group_in_table->getInfo();
    int32 redispatch_cycle = 0;
    while (true) {
        
        int32 move_len = login->moveToTemp(&temp_login, login->getLen());
        if (move_len != -1) {
            info->m_new_add_node_num = dcr::g_getStrategy()->addComputeNode(&temp_login, group_in_table);
        }
        temp_login.tempReleaseToGlobalFree();
        
        
        move_len = result->moveToTemp(&temp_result, result->getLen());
        info->m_processed_result_num = 0;
        if (move_len != -1 && move_len != 0) {
            info->m_processed_result_num = dcr::g_getStrategy()->processResult(&temp_result, group_in_table);
        }
        temp_result.tempReleaseToGlobalFree();
        group_task_free->tempReleaseToGlobalFree();
        
        
        if (redispatch_cycle < dcr::g_getConfig()->getRedispatchTime()) {
            redispatch_cycle ++;
        } else {
            redispatch_cycle = 0;
            info->m_redispatch_num_one_cycle = dcr::g_getStrategy()->redispatchTask(decompose_task, group_in_table);
            info->m_redispatch_num_all_cycles += info->m_redispatch_num_one_cycle;
            info->m_redispatch_num_one_cycle = 0;
            
        }
        
        if (redispatch_cycle == dcr::g_getConfig()->getRedispatchTime() - 1) {
            if (checkFinish(group_in_table)) {
                cout << "check finish! " << endl;
                int32 node_index = active_compute_node->getHead();
                ComputeNodeInfo *node = NULL;
                while (node_index != -1) {
                    node = static_cast<ComputeNodeInfo*>(active_compute_node->getMemoryBody(node_index));
                    UdpEndWork end;
                    end.endWork = true;
                    dcr::g_getSchedulerCommunicator()->udpEndWork(node->ip, &end);
                    int32 next = active_compute_node->getFqNextByIndex(node_index);
                    node_index = next;
                }
                Dispatcher::m_lock.Lock();
                LOG(INFO) << " group " << group_seq << " exit! " << endl;
                Dispatcher::m_lock.unLock();
                return;
            }
        }
        
        int32 node_index = active_compute_node->getHead();
        info->m_send_task_num = 0;
        info->m_load_task_num = 0;
        info->m_disconnect_node_per_cycle = 0;
        vector<string> delete_ip_list;
        bool delete_flag = false;
        while (node_index != -1) {
            ComputeNodeInfo *node = (ComputeNodeInfo*)active_compute_node->getMemoryBody(node_index);
            if (dcr::g_getStrategy()->checkDisconnect(node)) {
                info->m_disconnect_node_per_cycle++;
                char delete_ip[16];
                memcpy(delete_ip, node->ip, 16);
                delete_ip_list.push_back(delete_ip);
                dcr::g_getStrategy()->deleteComputeNode(node_index, decompose_task, group_in_table);
                delete_flag = true;
            } else {
                //注意此处的加锁，与普通的调度算法区别
                this->m_mutex->Lock();
                info->m_load_task_num += dcr::g_getStrategy()->loadTask(node, decompose_task, group_seq);
                this->m_mutex->unLock();
                info->m_send_task_num += dcr::g_getStrategy()->sendTask(node, node_index, group_in_table);
            }
            node_index = active_compute_node->getFqNextByIndex(node_index);
        }
        if (delete_flag) {
            dcr::g_getXMLLog()->add_DeleteComputeNode(delete_ip_list);
            LOG(INFO) << " delete compute node";
            delete_ip_list.clear();
        }
        info->m_send_task_num_all += info->m_send_task_num;
        info->m_load_task_num_all += info->m_load_task_num;
        info->m_disconnect_node_num += info->m_disconnect_node_per_cycle;
        info->m_active_node_num = active_compute_node->getLen();
        
        Dispatcher::m_lock.Lock();
        cout << "--------------------------------- Group " << group_seq << " Info -----------------------------------"<<endl;
        cout << "Acitve_cn:\t"<<info->m_active_node_num << "\tAdded_cn:\t"<< info->m_new_add_node_num<<"\tDisconnent_all: \t"
        << info->m_disconnect_node_num << "\tResult_packet:\t" << info->m_processed_result_num << endl;
        
        cout << "Load_task:\t" << info->m_load_task_num << "\tSend_task:\t" << info->m_send_task_num << "\tRedispatch_one:\t"
        << info->m_redispatch_num_one_cycle  << "\tDisconnect_one\t" << info->m_disconnect_node_per_cycle <<endl;
        
        cout << "Decompser queue len: \t" << dcr::g_getDecomposer()->getDecomposeQueue()->getLen() << "\tload_task_all\t"
        << info->m_load_task_num_all << " \tSend_task_all\t " << info->m_send_task_num_all << "\tTotal_redisp:\t"<<info->m_redispatch_num_all_cycles<< endl;
        cout <<"---------------------------------------------------------------------------------------" << endl;
        Dispatcher::m_lock.unLock();
        
        sleep(dcr::g_getConfig()->getSchedulerTime());
    }

}



int32 StrategyType::loadTask(ComputeNodeInfo *node, FunctionalQueue *decompose_queue, int32 group_seq) {
    CHECK_NOTNULL(node);
    CHECK_NOTNULL(decompose_queue);
    int32 wait_queue_len = node->m_task_wait_to_send.getLen();
    int32 dispatch_num = this->computeDispatchNum(wait_queue_len);
    int32 ret_val = 0;
    
    FunctionalQueue temp_load;
    if(!(temp_load.init(dcr::g_getComputationTaskQueue().get(), FQ_TASK_TEMP))) {
        LOG(ERROR) << " temp_load init failed: ";
        return 0;
    }

    unsigned long ip = inet_addr(node->ip);
    //LOG(ERROR) << " node ip is " << node->ip;
    unsigned long localhost = inet_addr("127.0.0.1");
    if(ip == localhost) {
        ip = inet_addr(dcr::g_getConfig()->getSchedulerIP());
    }
    if (m_ip_type.find(ip) == m_ip_type.end()) {
        LOG(ERROR) << " 节点未登记.";

    }
    unordered_set<int32>::iterator type_it = m_ip_type[ip]->begin();
    for ( ; ret_val < dispatch_num && m_type_index.size() != 0; ret_val++) {
        
        if (type_it ==  m_ip_type[ip]->end()) {
            //如果该节点没有类型，则不分配任务。
            LOG(WARNING) << " no this type ";
            //ret_val --;
            break;
            
        } else {
            
            int32 type = *type_it;
            size_t count = m_type_index.count(type);
            while (count == 0) {
                type_it++;
                if (type_it ==  m_ip_type[ip]->end()) {
                    break;
                }
                
                type = *type_it;
                count = m_type_index.count(type);
            }
            
            if (type_it ==  m_ip_type[ip]->end()) {
                //ret_val--;
                break;
            }
            
            unordered_map<int32, int32>::iterator find_it = m_type_index.find(type);
            //LOG(WARNING) << " type: " << type << " counting: " << count;
            node->m_mutex.Lock();
            int err = decompose_queue->moveOneElementTo(&(node->m_task_wait_to_send), find_it->second);
            node->m_mutex.unLock();
            if(err == 0) {
                LOG(ERROR) << " error index: " << find_it->second;
                ret_val--;
                continue;
            }
            m_type_index.erase(find_it);
            
            if (m_type_index.count(type) == 0) {
                //m_type_history[group_seq][ip]->erase(find_it->first);
                //m_type_history[group_seq][ip]->insert(find_it->first);
                //type_it = node->m_type_prior.begin();
            }
            
        }
        
    }
    
    return ret_val;
}


bool StrategyType::redispatchOneNode(ComputeNodeInfo *node, FunctionalQueue *decompose_queue,
                           time_t now, int32 &redisp_one_node, GroupInTable* group) {

    CHECK_NOTNULL(node);
    int32 already_send_index = node->m_task_already_send.getHead();
   
    unit_index_t *unit_index = NULL;
    while (already_send_index != -1) {
        unit_index = node->m_task_already_send.getOneUnitIndex(already_send_index);
        int next = node->m_task_already_send.getFqNextByIndex(already_send_index);
        if (difftime(now, unit_index->time) > dcr::g_getConfig()->getTaskLostTime()) {
            UdpSendTask *udp_task = (UdpSendTask*)node->m_task_already_send.getMemoryBody(already_send_index);
            Dispatcher::m_lock.Lock();
            LOG(WARNING) << " Redispatch Task: IP: " << node->ip << " group_index: " << group->m_group_seq
            << " node_index: " << udp_task->task_head.compute_node_index
            << " task_index: " << udp_task->task_head.mq_task_index << " task_id:" << udp_task->task_head.task_ID
            << " original task_id: " << udp_task->task_head.original_task_ID;
            Dispatcher::m_lock.unLock();
            
            node->m_mutex.Lock();
            
            //如果重发包，就需要将其ID换成新的不重复的ID，以免接收到旧的结果包。
            udp_task->task_head.task_ID = redispatch_index ++;
            
            int32 err = node->m_task_already_send.moveOneElementTo(decompose_queue, already_send_index);
            ComputationTask* compute_task = dcr::g_createComputationTask();
            CHECK_NOTNULL(compute_task);
            int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
            memcpy(compute_task, (char*)udp_task + offset, dcr::g_getComputeTaskSize());
            this->m_type_index.insert(make_pair(compute_task->type, (int32)already_send_index));
            delete compute_task;
            if (err == 0) {
                LOG(ERROR) << " error index: " << already_send_index;
            }
            node->m_mutex.unLock();

            int32 original_task_ID = udp_task->task_head.original_task_ID;
            Task::m_lock.Lock();
            dcr::g_getTaskManager()->getTaskByID(original_task_ID)->redispatch_packet_num ++;
            Task::m_lock.unLock();

            redisp_one_node++;
        } else {
            break;
        }
        already_send_index = next;
    }
    return true;

}


int32 StrategyType::deleteComputeNode(int32 delete_index, FunctionalQueue *decomposer_queue, GroupInTable* group) {
    ComputeNodeInfo *node = (ComputeNodeInfo*)group->m_active_compute_node->getMemoryBody(delete_index);
    unordered_set<unsigned long>::iterator find_end = dcr::g_getComputeNodeTable()->active_ip_list.end();
    
    if (dcr::g_getComputeNodeTable()->active_ip_list.find(inet_addr(node->ip)) != find_end) {
        

        int32 wait_to_send_index =  node->m_task_wait_to_send.getHead();
        node->m_mutex.Lock();
        while (wait_to_send_index != -1) {
            int32 next = node->m_task_wait_to_send.getFqNextByIndex(wait_to_send_index);
            UdpSendTask *udp_task = (UdpSendTask*)node->m_task_wait_to_send.getMemoryBody(wait_to_send_index);
            int32 err = node->m_task_wait_to_send.moveOneElementTo(decomposer_queue, wait_to_send_index);

            ComputationTask* compute_task = dcr::g_createComputationTask();
            CHECK_NOTNULL(compute_task);
            int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
            memcpy(compute_task, (char*)udp_task + offset, dcr::g_getComputeTaskSize());
            this->m_type_index.insert(make_pair(compute_task->type, (int32)wait_to_send_index));
            delete compute_task;
            if (err == 0) {
                LOG(ERROR) << " error wait to send index: " << wait_to_send_index;
            }
            wait_to_send_index = next;
        }
        node->m_mutex.unLock();

        int32 already_send_index =  node->m_task_already_send.getHead();
        node->m_mutex.Lock();
        while (already_send_index != -1) {
            int32 next = node->m_task_already_send.getFqNextByIndex(already_send_index);
            UdpSendTask *udp_task = (UdpSendTask*)node->m_task_already_send.getMemoryBody(already_send_index);
            int32 err = node->m_task_already_send.moveOneElementTo(decomposer_queue, already_send_index);

            ComputationTask* compute_task = dcr::g_createComputationTask();
            CHECK_NOTNULL(compute_task);
            int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
            memcpy(compute_task, (char*)udp_task + offset, dcr::g_getComputeTaskSize());
            this->m_type_index.insert(make_pair(compute_task->type, already_send_index));
            delete compute_task;
            if (err == 0) {
                LOG(ERROR) << " error already send index: " << already_send_index;
            }
            already_send_index = next;
        }
        node->m_mutex.unLock();

        FunctionalQueue *global_free = group->m_compute_node_queue->getGlobalFree();
        group->m_active_compute_node->moveOneElementTo(global_free, delete_index);
        dcr::g_getComputeNodeTable()->active_ip_list.erase(inet_addr(node->ip));
        return 1;
    } else {
        return 0;
    }
}
