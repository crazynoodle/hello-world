#include "caculator.h"
#include "system_init.h"
#include "dcr_base_class.h"
#include "../util/scoped_ptr.h"

Mutex Caculator::m_lock;
Mutex Job::m_lock;
map<string, string> Caculator::m_cache;
list<string> Caculator::cache_sequence;
bool Caculator::startWork() {
    Caculator::m_lock.init();
    Job::m_lock.init();
    for (int32 i = 0; i < dcr::g_getConfig()->getCalThreadNum(); i++) {
        int32 err = pthread_create(&this->m_caculate_thread[i], NULL, caculateProcess, this);
        if (err != 0) {
            LOG(ERROR) << "create caculate thread failed.";
            return false;
        }
        usleep(200);
    }
    return true;
}

bool Caculator::endWork() {
    signal(SIGCHLD, SIG_IGN);
    pthread_join(dcr::g_getComputeCommunicator()->m_end_work, NULL);
    int32 err = 0;
    for (int32 i = 0; i < dcr::g_getConfig()->getCalThreadNum(); i++) {
        err = pthread_cancel(m_caculate_thread[i]);
        if (err != 0) {
            LOG(ERROR) << " cancel caculate thread failed. " << "thread ID: " << i;
        }
    }
    
    if (err != 0) {
        return false;
    } else {
        return true;
    }
}

void* Caculator::caculateProcess(void *arg) {
    FunctionalQueue temp_task;
    temp_task.init(dcr::g_getTaskQueueOfComputeNode().get(), FQ_TASK_TEMP);
    FunctionalQueue* task_queue = dcr::g_getComputeCommunicator()->getTaskQueue();
    scoped_ptr<Job> compute_job (dcr::g_createJob());
    while (true) {
        if (task_queue->getLen() > 0) {
            
            Caculator::m_lock.Lock();
            //LOG(INFO) << " task_queue_len : " << task_queue->getLen();
            std::cout << " task queue len : " << task_queue->getLen() << std::endl;
            Caculator::m_lock.unLock();
            
            int32 ret_val = task_queue->moveToTemp(&temp_task, 1);
            if (ret_val != 1) {
                temp_task.tempReleaseToGlobalFree();
                continue;
            }
            //获取计算任务
            int32 head_index = temp_task.getHead();
            UdpSendTask* udp_task = (UdpSendTask*)temp_task.getMemoryBody(head_index);
            ComputationTask* compute_task = dcr::g_createComputationTask();
            CHECK_NOTNULL(compute_task);
            
            int32 offset = dcr::g_getUdpTaskSize() - dcr::g_getComputeTaskSize();
            memcpy(compute_task, (char*)udp_task + offset, dcr::g_getComputeTaskSize());
            //获取原始任务
            int32 original_task_ID = udp_task->task_head.original_task_ID;
            
            OriginalTask* original_task = dcr::g_getComputeCommunicator()->getOriginalTask(original_task_ID);
            CHECK_NOTNULL(original_task);
            //进行计算
            scoped_ptr<Result> compute_result(compute_job->computation(compute_task, original_task));
            if (compute_result.get() == NULL) {
                temp_task.tempReleaseToGlobalFree();
                LOG(WARNING) << " get null result! task id: " << udp_task->task_head.task_ID
                             << " original task id: " << original_task_ID;
                continue;
            }
            //结果返回
            dcr::g_getComputeCommunicator()->sendResult(compute_result.get(), udp_task);
            temp_task.tempReleaseToGlobalFree();
            
        }else {
            sleep(2);
            continue;
        }
    }
    return NULL;
}