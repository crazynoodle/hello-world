/*!
 * \file flags.h
 * \brief 参数解析模块
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */
#ifndef DCR_flags_h
#define DCR_flags_h

#include "../gflags/gflags.h"
#include "../util/common.h"
#include "../util/logger.h"
#include "../dcr/dcr_base_class.h"
#include "../network/network_packet.h"
#include "../dcr/strategy.h"
//#include "../dcr/task_manager.h"

#include <string>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>
#include <stdlib.h>

///
/// \brief 利用gflags对参数进行解析的全局函数
///
namespace dcr {
    
    ///
    /// \brief 对输入的参数进行正确性验证
    ///
    bool g_validateCommandLineFlags();
    
    ///
    /// \brief 获取用于任务文件名
    ///
    string g_getTaskFileName();

    ///
    /// \brief 获取粒度文件名
    ///
    string g_getGranulityFileName();
    
    ///
    /// \brief 根据参数判断本节点是不是调度节点
    ///
    bool g_isSchedulerNode();
    
    ///
    /// \brief 获取本节点的计算类型
    ///
    const char* g_workType();
    
    ///
    /// \brief 获取用户定义的原始任务的名字
    ///
    string g_getOriginalTaskName();
    
    ///
    /// \brief 获取用户定义的分解器的名字
    ///
    string g_getDecomposerName();
    
    ///
    /// \brief 获取用户定义的计算任务的名字
    ///
    string g_getComputationTaskName();
    
    ///
    /// \brief 获取用户定义的粒度的名字
    ///
    string g_getGranulityName();
    
    ///
    /// \brief 获取用户定义的结果名字
    ///
    string g_getResultName();
    
    ///
    /// \brief 获取用户定义Job的名字
    ///
    string g_getJobName();
    
    ///
    /// \brief 获取指定的调度策略
    ///
    int32 g_getStrategyType();

    ///
    /// \brief 获取节点类型文件名字
    ///
    string g_getNodeTypeFileName();
    
    ///
    /// \brief 获取任务类名字
    ///
    string g_getTaskName();

    ///
    /// \brief 获取日志系统名字
    ///
    string g_getXMLLogName();

    ///
    /// \brief 本节点主机名
    ///
    string g_getHostName();
    
    ///
    /// \brief 获取用户名
    ///
    string g_getUserName();
    
    ///
    /// \brief 获取当前时间
    ///
    string g_printCurrentTime();
    
    ///
    /// \brief 根据上面三个函数得到日子文件名
    ///
    string g_logFilePrefix();
    
    
    ///
    /// \brief 创建用户定义的分解器
    ///
    Decomposer* g_createDecomposer();
    
    ///
    /// \brief 创建用户定义的原始任务
    ///
    OriginalTask* g_createOriginalTask();
    
    ///
    /// \brief 创建用户定义的计算任务
    ///
    ComputationTask* g_createComputationTask();
    
    ///
    /// \brief 创建用户定义的粒度
    ///
    Granularity* g_createGranulity();
    
    ///
    /// \brief 创建用户定义的计算结果
    ///
    Result* g_createResult();
    
    ///
    /// \brief 创建用户定义的Job
    ///
    Job* g_createJob();
    
    ///
    /// \breif 创建调度策略
    ///
    Strategy* g_createStrategy();

    ///
    /// \breif 创建任务类
    ///
    Task* g_createTask();

    ///
    /// \breif 创建日志系统类
    ///
    XMLLog* g_createXMLLog();
    
    ///
    /// \brief 获取用户定义的原始任务的大小
    ///
    int32 g_getOriginalTaskSize();
    
    ///
    /// \brief 获取udpTask的大小
    ///
    int32 g_getUdpTaskSize();
    
    ///
    /// \brief 获取用户定义的计算任务的大小
    ///
    int32 g_getComputeTaskSize();
    
    ///
    /// \brief 获取udpResult的大小
    ///
    int32 g_getUdpResultSize();
    
    ///
    /// \brief 获取用户定义的结果的大小
    ///
    int32 g_getComputeResultSize();
    
}


#endif
