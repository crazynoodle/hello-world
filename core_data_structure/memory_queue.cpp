#include "../core_data_structure/memory_queue.h"

bool MemoryQueue::init(memory_queue_t mq_type, int32 unit_size, int32 queue_len) {
    
    CHECK_GT(unit_size, 0);
    CHECK_GT(queue_len, 0);
    
    this->m_type = mq_type;
    this->m_queue_len = queue_len;
    this->m_unit_size = unit_size;
    this->m_mutex = new Mutex();
    this->m_mutex->init();
    this->m_fun_queue_list_head = NULL;
    this->m_global_free = new FunctionalQueue();
    this->m_global_free->init(this, FQ_GLOBAL_FREE, 0, queue_len, 0, queue_len-1);
    try {
        this->m_queue = ::operator new(queue_len * unit_size);
    } catch (bad_alloc&) {
        LOG(ERROR) << "alloc task_queue buffer failed";
        delete m_mutex;
        delete m_global_free;
        return false;
    }
    
    try {
        this->m_unit_index = new unit_index_t[queue_len];
    } catch (bad_alloc&) {
        delete m_mutex;
        delete m_global_free;
        ::operator delete(m_queue);
        LOG(ERROR) << "alloc unit index buffer failed";
        return false;
    }
    
    for (int32 i = 0; i < queue_len; i++) {
        this->m_unit_index[i].prev = i - 1;
        this->m_unit_index[i].next = i + 1;
        this->m_unit_index[i].fun_queue_pointer = this->m_global_free;
    }
    this->m_unit_index[queue_len-1].next = -1;
    
    return true;
}

void MemoryQueue::destroyRealQueue(){
    delete m_global_free;
    m_global_free = NULL;
    delete m_mutex;
    m_mutex = NULL;
    ::operator delete (m_queue);
    m_queue = NULL;
    delete m_unit_index;
    m_unit_index = NULL;
}

void* MemoryQueue::getBody() {
    return this->m_queue;
}

void* MemoryQueue::getOneBodyByIndex(int32 index) {
    CHECK_GE(index, 0);
    CHECK_LE(index, this->m_queue_len);
    void* ret_val = this->m_queue;
    CHECK_NOTNULL(ret_val);
    return ((char*)ret_val + index * this->m_unit_size);
}

unit_index_t* MemoryQueue::getUnitIndex() {
    return this->m_unit_index;
}

unit_index_t* MemoryQueue::getOneUnitIndexByIndex(int32 index) {
    CHECK_GE(index, 0);
    CHECK_LE(index, this->m_queue_len);
    unit_index_t* ret_val = this->m_unit_index;
    CHECK_NOTNULL(ret_val);
    return (&ret_val[index]);
}

int32 MemoryQueue::getQueueLen() {
    return this->m_queue_len;
}

FunctionalQueue* MemoryQueue::getGlobalFree() {
    return this->m_global_free;
}

FunctionalQueue* MemoryQueue::getFunQueueListHead(){
    return this->m_fun_queue_list_head;
}

void MemoryQueue::setFunQueueListHead(FunctionalQueue* head){
    this->m_fun_queue_list_head = head;
}