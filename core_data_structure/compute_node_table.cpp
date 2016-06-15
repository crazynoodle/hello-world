#include "../core_data_structure/compute_node_table.h"
#include "../util/logger.h"
#include "../util/mutex.h"
#include "../util/scoped_ptr.h"
#include "../network/scheduler_communicator.h"
#include "../dcr/system_init.h"


bool ComputeNodeInfo::init(char* cn_ip, time_t now_time, int32 task_len) {
    memcpy(this->ip, cn_ip, 16);
    this->m_last_access = now_time;
    this->m_cq_task_len = task_len;
    this->m_task_already_send.init(dcr::g_getComputationTaskQueue().get(), FQ_TO_DELETE);
    this->m_task_wait_to_send.init(dcr::g_getComputationTaskQueue().get(), FQ_TO_SEND);
    this->m_mutex.init();
    return true;
}

void ComputeNodeInfo::destroy() {
}


bool GroupInfo::init() {
    this->m_active_node_num = 0;
    this->m_new_add_node_num = 0;
    this->m_disconnect_node_num = 0;
    this->m_disconnect_node_per_cycle = 0;
    this->m_load_task_num_all = 0;
    this->m_send_task_num_all = 0;
    this->m_load_task_num = 0;
    this->m_send_task_num = 0;
    this->m_processed_result_num = 0;
    this->m_redispatch_num_one_cycle = 0;
    this->m_redispatch_num_all_cycles = 0;
    return true;
}

void GroupInfo::destroy() {
    
}


bool GroupInTable::init(int32 num_per_group, int32 group_seq) {
    try {
        this->m_compute_node_queue = new MemoryQueue();
        this->m_info = new GroupInfo();
        this->m_login = new FunctionalQueue();
        this->m_result = new FunctionalQueue();
        this->m_active_compute_node = new FunctionalQueue();
        this->m_group_free = new FunctionalQueue();
    } catch (bad_alloc&) {
        LOG(ERROR) << "alloc buffer failed in group " << group_seq;
        return false;
    }
    
    this->m_group_seq = group_seq;
    
    if (!(this->m_compute_node_queue->init(MQ_COMPUTE_NODE, sizeof(ComputeNodeInfo), num_per_group))) {
        LOG(ERROR) << "init compute_node_queue failed in group " << group_seq;
        return false;
    }
    
    this->m_info->init();
    
    if (!(this->m_login->init(dcr::g_getLoginQueue().get(), FQ_LOGIN, group_seq))) {
        LOG(ERROR) << "init FQ_LOGIN failed in group " << group_seq;
        return false;
    }
    if (!(this->m_result->init(dcr::g_getResultQueue().get(), FQ_RESULT, group_seq))) {
        LOG(ERROR) << "init FQ_RESULT failed in group " << group_seq;
        return false;
    }
    if (!(this->m_active_compute_node->init(this->m_compute_node_queue, FQ_ACTIVE_CN, group_seq))) {
        LOG(ERROR) << "init FQ_ACTIVE_CN failed in group " << group_seq;
        return false;
    }
    if (!(this->m_group_free->init(dcr::g_getComputationTaskQueue().get(), FQ_GROUP_FREE, group_seq))) {
        LOG(ERROR) << "init FQ_GROUP_FREE failed in group " << group_seq;
        return false;
    }
    
    this->m_compute_node_len = num_per_group;
    return true;
}

void GroupInTable::destroy() {
    delete m_compute_node_queue;
    m_compute_node_queue = NULL;
    delete m_info;
    m_info = NULL;
    delete m_login;
    m_login = NULL;
    delete m_result;
    m_result = NULL;
    delete m_active_compute_node;
    m_active_compute_node = NULL;
    delete m_group_free;
    m_group_free = NULL;
}

MemoryQueue *GroupInTable::getComputeNodeQueue() {
    return m_compute_node_queue;
}

uint32 GroupInTable::getComputeNodeLen() {
    return m_compute_node_len;
}


GroupInfo *GroupInTable::getInfo() {
    return m_info;
}

FunctionalQueue *GroupInTable::getLogin() {
    return m_login;
}

FunctionalQueue *GroupInTable::getResult() {
    return m_result;
}

FunctionalQueue *GroupInTable::getActiveComputeNode() {
    return m_active_compute_node;
}

FunctionalQueue *GroupInTable::getGroupFree() {
    return m_group_free;
}

bool ComputeNodeTable::init(int32 group_num, int32 num_per_group) {
    
    CHECK_GT(group_num, 0);
    CHECK_GT(num_per_group, 0);
    
    this->m_group_num = group_num;
    
    try {
        this->m_group = new GroupInTable[group_num];
    } catch (bad_alloc&) {
        LOG(ERROR) << " alloc group failed ";
        return false;
    }
    
    for (int32 i = 0; i < group_num; i++) {
        if(!(this->m_group[i].init(num_per_group, i))) {
            LOG(ERROR) << " init compute node table failed ";
            return false;
        }
    }
    
    return true;
}

void ComputeNodeTable::destroy() {
    delete []m_group;
}

int32 ComputeNodeTable::getGroupNum() {
    return m_group_num;
}

GroupInTable* ComputeNodeTable::getGroup(int32 group_index) {
    CHECK_LE(group_index, m_group_num);
    CHECK_GE(group_index, 0);
    return &m_group[group_index];
}

