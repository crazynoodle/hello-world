/*!
 * \file dispatcher.h
 * \brief 调度节点分发器
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef __DCR__dispacher__
#define __DCR__dispacher__

#include "../util/common.h"
#include "../util/scoped_ptr.h"
#include "../core_data_structure/memory_queue.h"
#include "../core_data_structure/functional_queue.h"
#include "../core_data_structure/compute_node_table.h"
#include "../dcr/dcr_base_class.h"
#include "../util/config.h"

#include <unistd.h>
#include <pthread.h>

namespace dcr {
    scoped_ptr<SystemConfig>& g_getConfig();
    scoped_ptr<Strategy>& g_getStrategy();
};

///
/// \brief 从分解器加载计算任务, 并分发到计算节点
///
class Dispatcher {
public:
    Dispatcher() {
        
    };
    
    ~Dispatcher() {
        this->destroy();
    };
    
    ///
    ///\brief 用于分配空间
    ///
    /// 根据分组数创建分配线程数组
    bool init() {
        int32 group_num = dcr::g_getConfig()->getNodeGroup();
        try {
            this->m_dispach_group = new pthread_t[group_num];
        } catch (bad_alloc&) {
            LOG(ERROR) << " can not init dispacher! ";
            return false;
        }
        
        return true;
    }
    
    ///
    /// \brief 用于启动分发线程
    ///
    bool startWork(int32 group_num);
    
    ///
    /// \brief 等待分发线程退出结束
    ///
    bool endWork(int32 group_num);
    
private:
    /// \brief 分发线程主函数
    ///
    /// 1.登陆队列的包移入active_node 队列
    /// 2.结果队列处理
    /// 3.如果到了检查redispatch周期，则进行检查和redispatch, 并检查所有任务是否都已经完成
    ///   如果任务已经全部完成则向计算节点发送结束包返回
    /// 4.检查active_compute_node里的每个元素是否disconnected，如果没有则加载和发送任务
    /// 5.打印调试信息
    /// 6.sleep TIME_SLICE
    static void *dispatch(void *arg);
    
    bool destroy() {
        delete []m_dispach_group;
        return true;
    }
    
    pthread_t* m_dispach_group; ///< 分发线程ID
    
    static Mutex m_lock;
    
    friend class Strategy;
    friend class StrategyBlast;
    friend class StrategyType;
};

#endif 
