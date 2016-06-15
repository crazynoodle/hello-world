#include "../core_data_structure/functional_queue.h"
#include "../core_data_structure/memory_queue.h"
bool FunctionalQueue::init(MemoryQueue *memory_queue, fun_queue_t type, int32 group_seq,
                           int32 len, int32 head, int32 tail){
    
    CHECK_NOTNULL(memory_queue);
    CHECK_GE(len, 0);
    this->m_type = type;
    this->m_group_seq = group_seq;
    this->m_len = len;
    this->m_head = head;
    this->m_tail = tail;
    this->m_memory_queue = memory_queue;
    this->m_mutex = new Mutex();
    this->m_mutex->init();
    
    this->m_mutex->Lock();
    this->m_next_fun_queue = memory_queue->getFunQueueListHead();
    this->m_memory_queue->setFunQueueListHead(this);
    this->m_mutex->unLock();
    
    return true;
}

void FunctionalQueue::destroy() {
    delete m_mutex;
}

int32 FunctionalQueue::moveToTemp(FunctionalQueue *dest, int32 move_len) {
    CHECK_NOTNULL(dest);
    CHECK_GE(move_len, 0);
    if (move_len == 0 || this->m_len == 0) {
        return 0;
    }
    MemoryQueue *memory_queue_temp = this->m_memory_queue;
    CHECK_NOTNULL(memory_queue_temp);
    int32 mq_len = memory_queue_temp->getQueueLen();
    CHECK_GE(mq_len, 0);
    unit_index_t *unit_index = memory_queue_temp->getUnitIndex();
    CHECK_NOTNULL(unit_index);
    int32 real_move_len = 0;
    int32 head_index = -1;
    int32 tail_index = -1;
    
    this->m_mutex->Lock();
    if (this->m_len <= move_len) {
        real_move_len = this->m_len;
        head_index = this->m_head;
        tail_index = this->m_tail;
        this->m_len = 0;
        this->m_head = -1;
        this->m_tail = -1;
    } else {
        real_move_len = move_len;
        head_index = this->m_head;
        tail_index = this->m_head;
        for (int32 i = 0; i < move_len - 1; i++) {
            if ((tail_index < 0 ) || (tail_index >= mq_len)) {
                LOG(ERROR) << "bad chain in functional queue";
                return -1;
            }
            tail_index = unit_index[tail_index].next;
        }
        this->m_len -= real_move_len;
        this->m_head = unit_index[tail_index].next;
        unit_index[this->m_head].prev = -1;
    }
    this->m_mutex->unLock();
    
    int32 index_temp = head_index;
    for (int32 j = 0; j < real_move_len; j++) {
        unit_index[index_temp].fun_queue_pointer = dest;
        index_temp = unit_index[index_temp].next;
    }
    
    if (dest->m_len == 0) {
        dest->m_len = real_move_len;
        dest->m_head = head_index;
        dest->m_tail = tail_index;
        unit_index[head_index].prev = -1;
        unit_index[tail_index].next = -1;
    } else {
        dest->m_len += real_move_len;
        unit_index[dest->m_tail].next = head_index;
        unit_index[head_index].prev = dest->m_tail;
        unit_index[tail_index].next = -1;
        dest->m_tail = tail_index;
    }
    
    return real_move_len;
}

int32 FunctionalQueue::moveFromTemp(FunctionalQueue *src, int32 move_len) {
    CHECK_NOTNULL(src);
    CHECK_GE(move_len, 0);
    if (move_len == 0 || src->m_len == 0) {
        return 0;
    }
    MemoryQueue *memory_queue_temp = src->m_memory_queue;
    CHECK_NOTNULL(memory_queue_temp);
    int32 mq_len = memory_queue_temp->getQueueLen();
    CHECK_GE(mq_len, 0);
    unit_index_t *unit_index = memory_queue_temp->getUnitIndex();
    CHECK_NOTNULL(unit_index);
    
    int32 real_move_len = 0;
    int32 head_index = -1;
    int32 tail_index = -1;
    
    // 对src 队列进行处理
    if (src->m_len <= move_len) {
        real_move_len = src->m_len;
        head_index = src->m_head;
        tail_index = src->m_tail;
        src->m_len = 0;
        src->m_head = -1;
        src->m_tail = -1;
    } else {
        real_move_len = move_len;
        head_index = src->m_head;
        tail_index = src->m_head;
        for (int32 i = 0; i < move_len -1; i++) {
            if (tail_index < 0 || tail_index >= mq_len) {
                LOG(ERROR) << "bad chain in functional queue";
                return -1;
            }
            tail_index = unit_index[tail_index].next;
        }
        src->m_len -= real_move_len;
        src->m_head = unit_index[tail_index].next;
        unit_index[src->m_head].prev = -1;
    }
    
    int32 index_temp = head_index;
    for (int32 j = 0; j < real_move_len; j++) {
        unit_index[index_temp].fun_queue_pointer = this;
        index_temp = unit_index[index_temp].next;
    }
    
    // 对dest进行处理
    this->m_mutex->Lock();
    if (this->m_len == 0) {
        this->m_len = real_move_len;
        this->m_head = head_index;
        this->m_tail = tail_index;
        unit_index[this->m_head].prev = -1;
        unit_index[this->m_tail].next = -1;
    } else {
        this->m_len += real_move_len;
        
        unit_index[this->m_tail].next = head_index;
        unit_index[head_index].prev = this->m_tail;
        
        unit_index[tail_index].next = -1;
        this->m_tail = tail_index;
    }
    this->m_mutex->unLock();
    
    return real_move_len;
}

int32 FunctionalQueue::moveOneElementTo(FunctionalQueue *dest, int32 move_index) {
    CHECK_NOTNULL(dest);
    CHECK_GE(move_index, 0);
    if (dest == this) {
        return 0;
    }
    int32 src_queue_len = this->getLen();
    if (src_queue_len == 0) {
        LOG(ERROR) << " bad in para: src_queue_len = 0";
        return 0;
    }
    unit_index_t *unit_index = this->m_memory_queue->getOneUnitIndexByIndex(move_index);
    if (unit_index->fun_queue_pointer != this) {
        LOG(ERROR) << " bad in para: move_index wrong ";
        return 0;
    }
    
    if (src_queue_len == 1) {
        if ((this->getHead() != move_index) || (this->getTail() != move_index)) {
            LOG(ERROR) << " bad in para: ";
            return 0;
        }
        this->m_len = 0;
        this->m_head = this->m_tail = -1;
    } else {
        if (this->getHead() == move_index) {
            this->m_head = unit_index->next;
            unit_index_t *temp = this->m_memory_queue->getOneUnitIndexByIndex(this->m_head);
            temp->prev = -1;
        } else if(this->getTail() == move_index) {
            this->m_tail = unit_index->prev;
            unit_index_t *temp = this->m_memory_queue->getOneUnitIndexByIndex(this->m_tail);
            temp->next = -1;
        } else {
            unit_index_t *temp = this->m_memory_queue->getOneUnitIndexByIndex(unit_index->next);
            unit_index_t *temp1 = this->m_memory_queue->getOneUnitIndexByIndex(unit_index->prev);
            temp1->next = unit_index->next;
            temp->prev = unit_index->prev;
        }
        this->m_len--;
    }
    
    unit_index->fun_queue_pointer = dest;
    
    if(dest->m_len == 0) {
        dest->m_len = 1;
        dest->m_head = move_index;
        dest->m_tail = move_index;
        unit_index->prev = -1;
        unit_index->next = -1;
    } else {
        dest->m_len++;
        unit_index->prev = dest->m_tail;
        unit_index->next = -1;
        unit_index_t *temp = this->m_memory_queue->getOneUnitIndexByIndex(dest->m_tail);
        temp->next = move_index;
        dest->m_tail = move_index;
    }
    return 1;
}

int32 FunctionalQueue::tempAllocFromGlobalFree(int32 len) {
    FunctionalQueue *global_free = this->m_memory_queue->getGlobalFree();
    CHECK_NOTNULL(global_free);
    if (global_free->getLen() == 0) {
        LOG(INFO) << " no available space! ";
        return 0;
    }
    return global_free->moveToTemp(this, len);
}

int32 FunctionalQueue::tempReleaseToGlobalFree() {
    FunctionalQueue *global_free = this->m_memory_queue->getGlobalFree();
    CHECK_NOTNULL(global_free);
    return global_free->moveFromTemp(this, this->m_len);
}

bool FunctionalQueue::checkIndex(int32 index) {
    CHECK_GE(index, 0);
    int32 len = this->m_memory_queue->getQueueLen();
    CHECK_LE(index, len);
    unit_index_t *unit_index = this->m_memory_queue->getOneUnitIndexByIndex(index);
    CHECK_NOTNULL(unit_index);
    if (unit_index->fun_queue_pointer == this) {
        return true;
    } else {
        return false;
    }
}

void* FunctionalQueue::getMemoryBody(int32 index) {
    CHECK_GE(index, 0);
    int32 len = this->m_memory_queue->getQueueLen();
    CHECK_LT(index, len);
    void *body = this->m_memory_queue->getOneBodyByIndex(index);
    CHECK_NOTNULL(body);
    return body;
}

unit_index_t* FunctionalQueue::getOneUnitIndex(int32 index) {
    CHECK_GE(index, 0);
    int32 len = this->m_memory_queue->getQueueLen();
    CHECK_LT(index, len);
    unit_index_t* unit_index = this->m_memory_queue->getOneUnitIndexByIndex(index);
    CHECK_NOTNULL(unit_index);
    return unit_index;
}


MemoryQueue* FunctionalQueue::getMemoryQueue() {
    return this->m_memory_queue;
}

int32 FunctionalQueue::getGroupSeq() {
    return this->m_group_seq;
}

int32 FunctionalQueue::getLen() {
    int32 ret_val;
    this->m_mutex->Lock();
    ret_val = this->m_len;
    this->m_mutex->unLock();
    return ret_val;
}

int32 FunctionalQueue::getHead() {
    return this->m_head;
}

int32 FunctionalQueue::getTail() {
    return this->m_tail;
}

int32 FunctionalQueue::getFqNextByIndex(int32 fq_index) {
    MemoryQueue *mq = this->m_memory_queue;
    CHECK_NOTNULL(mq);
    unit_index_t *unit_index = mq->getUnitIndex();
    CHECK_NOTNULL(unit_index);
    int32 fq_next = unit_index[fq_index].next;
    return fq_next;
}

int32 FunctionalQueue::getFqPrevByIndex(int32 fq_index) {
    MemoryQueue *mq = this->m_memory_queue;
    CHECK_NOTNULL(mq);
    unit_index_t *unit_index = mq->getUnitIndex();
    CHECK_NOTNULL(unit_index);
    int32 fq_prev = unit_index[fq_index].prev;
    return fq_prev;
}

FunctionalQueue* FunctionalQueue::getNextFq() {
    return m_next_fun_queue;
}

void FunctionalQueue::lockFq() {
    this->m_mutex->Lock();
}

void FunctionalQueue::unlockFq() {
    this->m_mutex->unLock();
}

