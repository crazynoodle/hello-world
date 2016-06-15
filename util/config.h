/*!
 * \file config.h
 * \brief 系统配置管理类
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.11.31
 */

#ifndef __DCR__config__
#define __DCR__config__

#include "../tinyxml/tinyxml.h"
#include "../util/common.h"
#include "../util/logger.h"

#include <iostream>
using namespace std;

class SystemConfig {
public:
    SystemConfig() {
        
    }
    
    ~SystemConfig() {
        this->destroy();
    }
    
    bool init();
    
    char* getSchedulerIP();
    
    int32 getSchedulerTime();
    
    int32 getDisconnetTime();
    
    int32 getTaskLostTime();
    
    int32 getRedispatchTime();
    
    int32 getTaskQueueLen();
    
    int32 getResultQueueLen();
    
    int32 getLoginQueueLen();
    
    int32 getNodeGroup();
    
    int32 getNodePerGroup();
    
    int32 getCalThreadNum();
    
    int32 getTryConnectTime();
    
    int32 getHeartBeatTime();
    
    int32 getTaskQueuePerNode();

    string getVersion();

    string getMachine();

    string getTask_file_path();

    string getResult_file_path();
    
    void updateTaskLostTime(int lost_time);
	
    
private:
    bool destroy() {
        return true;
    }
    
    char m_scheduler_ip[16];                       ///< 调度节点IP
    string version;
    string machine;
    string task_file_path;
    string result_file_path;
    
    int32 m_sche_time_slice;                       ///< dispacher 每个调度周期sleep的时间
    int32 m_discon_time_slice;                     ///< 判断计算节点是否已经与调度节点失去联系的时间
    int32 m_task_lost_time_slice;                  ///< 判断发出去的任务是否已经丢失的时间
    int32 m_redispach_cycles;                      ///< 每隔多少个调度周期就要进行任务检查是否有任务丢失
    
    int32 m_task_queue_len;                        ///< 任务内存池的大小
    int32 m_result_queue_len;                      ///< 结果内存池的大小
    int32 m_login_queue_len;                       ///< 登陆包内存池的大小
    int32 m_node_group;                            ///< 将计算节点分为多少个分组
    int32 m_node_per_group;                        ///< 每个分组有多少个计算节点
    
    int32 m_caculte_thread_num;                    ///< 计算节点开启多少个计算线程
    int32 m_try_connect_time;                      ///< 计算节点尝试连接调度节点多少次后失败放弃
    int32 m_heart_beat_time_slice;                 ///< 计算节点隔多少秒给调度节点发送心跳包
    int32 m_task_queue_per_node;                   ///< 计算节点维持多长的队列
    
};

#endif 
