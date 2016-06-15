/*!
 * \file caculator.h
 * \brief 计算节点计算器
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */


#ifndef __DCR__caculator__
#define __DCR__caculator__

#include "../util/scoped_ptr.h"
#include "../util/common.h"
#include "../util/logger.h"
#include "../core_data_structure/functional_queue.h"
#include "../util/config.h"
#include <map>
#include <list>
#include <unistd.h>

namespace dcr {
    scoped_ptr<SystemConfig>& g_getConfig();
}

///
/// \brief 计算器，计算过程循环
///

class Caculator {
public:
    Caculator() {}
    
    ~Caculator() {
        this->destroy();
    }
    
    ///
    ///\breif 分配空间
    ///
    bool init() {
        int32 thread_num = dcr::g_getConfig()->getCalThreadNum();
        try {
            m_caculate_thread = new pthread_t[thread_num];
        } catch (bad_alloc&) {
            LOG(ERROR) << " init calculator failed! ";
            return false;
        }
        return true;
    }
    
    ///
    /// \brief 用于启动计算线程
    ///
    bool startWork();
    
    ///
    /// \brief 结束分发线程
    ///
    bool endWork();

    ///
    /// \brief 计算节点的缓存
    ///
    static map<string, string> m_cache;

    ///
    /// \brief 记录缓存加入的先后，实现LRU算法
    ///
    static list<string> cache_sequence;
    
private:
    /// \brief 计算线程主函数
    ///
    /// 1.取出一个计算任务
    /// 2.根据计算任务取出对应originalTask
    /// 3.进行计算
    /// 4.返回结果
    /// 5.循环1-4
    static void *caculateProcess(void *arg);
    
    bool destroy() {
        delete []m_caculate_thread;
        return true;
    }
    
    static Mutex m_lock;

    
    pthread_t* m_caculate_thread;  ///< 计算线程ID
};

#endif
