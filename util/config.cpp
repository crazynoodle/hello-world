#include "config.h"

#include <string>

bool SystemConfig::init() {
    string config_file = "./config.xml";
    TiXmlDocument pdoc(config_file.c_str());
    if (!(pdoc.LoadFile())) {
        LOG(ERROR) << " Load config file failed! ";
        return false;
    }
    TiXmlElement* root_element = pdoc.RootElement();
    CHECK_NOTNULL(root_element);

    TiXmlElement* head = root_element->FirstChildElement("Head");
    CHECK_NOTNULL(head);

    TiXmlElement* version_ele = head->FirstChildElement("Version");
    CHECK_NOTNULL(version_ele);
    //version = version_ele->GetText();
    const char* version_text = version_ele->GetText();
    if (version_text) {  
        version = version_text;  
    }
    
    TiXmlElement* machine_ele = head->FirstChildElement("Machine");
    CHECK_NOTNULL(machine_ele);
    machine = machine_ele->GetText();

    TiXmlElement* task_file_ele = head->FirstChildElement("TaskPath");
    CHECK_NOTNULL(task_file_ele);
    task_file_path = task_file_ele->GetText();
    
    TiXmlElement* result_file_ele = head->FirstChildElement("ResultPath");
    CHECK_NOTNULL(result_file_ele);
    result_file_path = result_file_ele->GetText();

    TiXmlElement* scheduler_node = root_element->FirstChildElement("SchedulerNode");
    CHECK_NOTNULL(scheduler_node);
    
    TiXmlElement* ip = scheduler_node->FirstChildElement("IP");
    CHECK_NOTNULL(ip);
    memset(m_scheduler_ip, 0, 16);
    strcpy(m_scheduler_ip, ip->GetText());
    
    TiXmlElement* cycles = scheduler_node->FirstChildElement("Cycles");
    CHECK_NOTNULL(cycles);
    
    TiXmlElement* sched_time = cycles->FirstChildElement("SchedulerTimeSlice");
    CHECK_NOTNULL(sched_time);
    m_sche_time_slice = atoi(sched_time->GetText());
    
    TiXmlElement* discon_time = cycles->FirstChildElement("DisconnectTimeSlice");
    CHECK_NOTNULL(discon_time);
    m_discon_time_slice = atoi(discon_time->GetText());
    
    TiXmlElement* task_lost = cycles->FirstChildElement("TaskLostTimeSLice");
    CHECK_NOTNULL(task_lost);
    m_task_lost_time_slice = atoi(task_lost->GetText());
    
    TiXmlElement* redispath_cycle = cycles->FirstChildElement("RedispachCycle");
    CHECK_NOTNULL(redispath_cycle);
    m_redispach_cycles = atoi(redispath_cycle->GetText());
    
    TiXmlElement* queue_len = scheduler_node->FirstChildElement("QueueLength");
    CHECK_NOTNULL(queue_len);
    
    TiXmlElement* task_queue_len = queue_len->FirstChildElement("TaskQueueMaxLen");
    CHECK_NOTNULL(task_queue_len);
    m_task_queue_len = atoi(task_queue_len->GetText());
    
    TiXmlElement* result_queue_len = queue_len->FirstChildElement("ResultQueueMaxLen");
    CHECK_NOTNULL(result_queue_len);
    m_result_queue_len = atoi(result_queue_len->GetText());
    
    TiXmlElement* login_queue_len = queue_len->FirstChildElement("LoginQueueMaxLen");
    CHECK_NOTNULL(login_queue_len);
    m_login_queue_len = atoi(login_queue_len->GetText());
    
    TiXmlElement* node_group = queue_len->FirstChildElement("NodeGroup");
    CHECK_NOTNULL(node_group);
    m_node_group = atoi(node_group->GetText());
    
    TiXmlElement* node_per_group = queue_len->FirstChildElement("NodePerGroup");
    CHECK_NOTNULL(node_per_group);
    m_node_per_group = atoi(node_per_group->GetText());
    
    TiXmlElement* compute_node = root_element->FirstChildElement("ComputeNode");
    CHECK_NOTNULL(compute_node);
    
    TiXmlElement* cal_thread = compute_node->FirstChildElement("CaculateThreadNum");
    CHECK_NOTNULL(cal_thread);
    m_caculte_thread_num = atoi(cal_thread->GetText());
    
    TiXmlElement* try_connect = compute_node->FirstChildElement("TryConnectTime");
    CHECK_NOTNULL(try_connect);
    m_try_connect_time = atoi(try_connect->GetText());
    
    TiXmlElement* heart_beat = compute_node->FirstChildElement("HeartBeatTimeSlice");
    CHECK_NOTNULL(heart_beat);
    m_heart_beat_time_slice = atoi(heart_beat->GetText());
    
    TiXmlElement* queue_len_cn = compute_node->FirstChildElement("TaskQueuePerComputeNode");
    CHECK_NOTNULL(queue_len_cn);
    m_task_queue_per_node = atoi(queue_len_cn->GetText());
    
    
    return true;
}

char* SystemConfig::getSchedulerIP() {
    return this->m_scheduler_ip;
}

int32 SystemConfig::getSchedulerTime() {
    return this->m_sche_time_slice;
}

int32 SystemConfig::getDisconnetTime() {
    return this->m_discon_time_slice;
}

int32 SystemConfig::getTaskLostTime() {
    return this->m_task_lost_time_slice;
}

int32 SystemConfig::getRedispatchTime() {
    return this->m_redispach_cycles;
}

int32 SystemConfig::getTaskQueueLen() {
    return this->m_task_queue_len;
}

int32 SystemConfig::getResultQueueLen() {
    return this->m_result_queue_len;
}

int32 SystemConfig::getLoginQueueLen() {
    return this->m_login_queue_len;
}

int32 SystemConfig::getNodeGroup() {
    return this->m_node_group;
}

int32 SystemConfig::getNodePerGroup() {
    return this->m_node_per_group;
}

int32 SystemConfig::getCalThreadNum() {
    return this->m_caculte_thread_num;
}

int32 SystemConfig::getTryConnectTime() {
    return this->m_try_connect_time;
}

int32 SystemConfig::getHeartBeatTime() {
    return this->m_heart_beat_time_slice;
}

int32 SystemConfig::getTaskQueuePerNode() {
    return this->m_task_queue_per_node;
}

string SystemConfig::getVersion() {
    return this->version;
}

string SystemConfig::getMachine() {
    return this->machine;
}

string SystemConfig::getTask_file_path() {
    return  this->task_file_path;
}

string SystemConfig::getResult_file_path() {
    return this->result_file_path;
}

void SystemConfig::updateTaskLostTime(int lost_time) {
    this->m_task_lost_time_slice = lost_time;
}