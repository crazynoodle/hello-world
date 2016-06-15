#include "../dcr/dispacher.h"
#include "../dcr/system_init.h"

//Mutex Dispatcher::m_lock;

bool Dispatcher::startWork(int group_num) {
    // 创建group_num个线程来进行任务分发
    for (int32 i = 0; i < group_num; i++) {
        int32 err = pthread_create(&m_dispach_group[i], NULL, dispatch, &i);
        if (err != 0) {
            LOG(ERROR) << "create dispatch thread failed";
            return false;
        }
        usleep(1000);
    }
    return true;
}

bool Dispatcher::endWork(int32 group_num) {
    //结束进程
    for (int32 i = 0; i < group_num; i++) {
        pthread_join(m_dispach_group[i], NULL);
    }
    return true;
}

void *Dispatcher::dispatch(void *arg) {
    
    int32 group_seq = *(int32*)arg;

    dcr::g_getStrategy()->dispachStrategy(group_seq);

    return (void*)NULL;
}