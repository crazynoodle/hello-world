/*!
 * \file dcr_base_class.h
 * \brief 供用户调用的基类
 *
 * 主要有Decomposer，OriginalTask, Granularity, ComputationTask
 * Job, Result这几个类
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */


#ifndef DCR_dcr_base_class_h
#define DCR_dcr_base_class_h

#include "../util/common.h"
#include "../util/scoped_ptr.h"
#include "../dcr/class_register.h"
#include "../core_data_structure/functional_queue.h"
#include "../tinyxml/tinyxml.h"
#include "../dcr/task_manager.h"

#include <string>
#include <map>
#include <vector>
#include <pthread.h>
using std::string;
class XMLLog;
class Job;
class Strategy;
class StrategyBlast;
class StrategyType;
namespace dcr {
    bool g_SchedulerNodeInitialize();
    bool g_SchedulerWork();
    bool g_SchedulerFinilize();
}


enum task_state {CREATE, RUN, FINISH};
class Task {
public:
    Task():task_id(0), task_name(""), task_file_name("/"), result_file_name("/"), task_creater("root"),
         send_packet_num(0), finish_packet_num(0), state(CREATE), redispatch_packet_num(0), decompose_packet_num(0)
    {}

    virtual ~Task() { this->destroy(); }

    virtual bool destroy();

    virtual string toString();

    int32 task_id;
    string task_name;
    string task_file_name;
    string result_file_name;
    string task_creater;
    int32 send_packet_num;
    int32 finish_packet_num;
    int32 redispatch_packet_num;
    int32 decompose_packet_num;
    task_state state;
    static Mutex m_lock; 

};


///
/// \brief 用于给用户描述分解完的计算任务的类
///
class ComputationTask {
public:
    ComputationTask() {};
    
    virtual ~ComputationTask() {};

    int32 type;

    int32 prior;
};

///
/// \brief 用于给用户描述初始任务的类
///
/// 用户需要实现给OriginalTask进行初始化的任务, 用XML文件进行初始化
class OriginalTask {
public:
    OriginalTask() {};
    
    virtual ~OriginalTask() {};
    
    virtual bool readFromFileInit(TiXmlElement* const task_element) = 0;

    ///
    /// \brief 根据原始任务文件来初始化结果文件
    ///
    virtual string createResultFile(TiXmlElement* const task_element, Task* task) = 0;
    
    virtual int32 getType() {
        return -1;
    }
};

///
/// \brief 用于给用户描述初始粒度的类
///
/// 用户需要实现给granularity成员进行初始化的任务，用XML文件进行初始化
class Granularity {
public:
    Granularity() {};

    virtual ~Granularity() {};
    
    virtual bool readFromFileInit(TiXmlElement* const granulity_element) = 0;
};

///
/// \brief 用于给用户描述计算结果的类
///
/// 用户需要实现给result成员进行初始化的任务,
/// reduce结果需要进行初始化，普通计算结果不用
class Result {
public:
    Result() {};
    
    virtual ~Result() {};
    
    virtual bool init() = 0;
};


class Resource{
public:
    Resource() {};

    virtual ~Resource() {};

};


///
/// \brief 用于给用户描述计算如何分解原始任务的类
///
/// 用户需要描述的是其中的虚函数, 即
/// 1.如何通过原始任务, 粒度获得分解获得计算任务
/// 2.分解结束的条件是什么
class Decomposer {
public:
    Decomposer() {};
    
    virtual ~Decomposer() {
        this->destroy();
    };
    
    ///
    /// \brief 分解是否结束
    ///
    virtual bool isFinishGetTask() = 0;
    
    /// \brief 如何分解获得一个计算任务
    /// \param original_task 用于分解的原始任务
    /// \param granulity 分解的粒度
    /// \param is_first_get 是否是第一次进入分解
    /// \return 分解得到的计算任务
    virtual ComputationTask *getTask(int32 original_task_ID, OriginalTask* original_task, Granularity* granulity, bool is_first_get) = 0;

    virtual pair<int32, int32> getTypePrior(OriginalTask* original_task,Granularity* granularity, ComputationTask* compute_task, bool is_first_get) = 0;

    ///
    /// \brief 分解器分解时调用的方法，描述如何更新结果文件。
    ///
    virtual void updateResultFileByDecomposer(OriginalTask* original_task, int32 original_task_ID, 
                                                    Granularity* granularity,  string result_file_path, bool is_first_get) = 0;
    
     Mutex* m_mutex;                                    ///< 分解器的线程锁


private:
    ///
    /// 分解线程函数
    ///
    /// 1.计算分解速度
    /// 2.初次分解
    /// 3.判断分解是否完成
    /// 4.判断是否已经达到本轮分解任务的数目，如果达到则进行sleep
    /// 5.如果没有则继续分解, 重复3-5
    static void *decompose(void *arg);
    
    ///
    /// 自动保存结果的线程
    ///
    static void *save_result(void *arg);
    
    ///
    /// \brief 插入原始任务
    ///
    int32 insertOriginalTask(OriginalTask* original_task);
    
    ///
    /// \brief 插入每个原始任务对应的粒度
    ///
    bool insertGranulity(Granularity *granulity);
    
    ///
    /// \brief 插入每个原始任务对应的最终结果
    ///
    bool insertFinalResult(Result *final_result);
    
    ///
    /// \brief 计算任务分解的速度
    ///
    int32 computeDecomposeSpeed();
    
    ///
    /// \brief 开启分解线程
    ///
    bool startWork();
    
    ///
    /// \brief 结束分解线程
    ///
    bool endWork();
    
    
    /// \brief 对分解器进行初始化
    /// 原注释：
    /// 读取XML文件，调用OriginalTask的虚函数进行初始化
    /// 有多少OriginalTask就有多少Granulity和final_task
    /// 
    /// 2015-09-21
    /// 当前功能
    /// 初始化线程锁
    /// 初始化用户分解队列
    bool init();
    
    ///
    /// \brief 销毁init中的内容
    ///
    bool destroy();

    int32 decomposeNewTask(Task *task);
    
    OriginalTask* getOriginalTaskByID(int32 ID);
    
    Granularity* getGranulityByID(int32 ID);
    
    Result* getFinalResultByID(int32 ID);
    
    int64 getDecomposeNumByID(int32 ID);
    
    int64 getResultNumByID(int32 ID);
    
    FunctionalQueue *getDecomposeQueue();
    
    map<int32, OriginalTask*> m_original_task_queue;   ///< 用户任务
    map<int32, Granularity*> m_granulity_queue;        ///< 每个用户任务对应的粒度
    map<int32, Result*> m_final_result;                ///< 每个用户任务对应的最终结果
    map<int32, int64> m_decompose_num;                 ///< 用于统计每个原始任务总共分解了多少个计算任务
    map<int32, int64> m_result_num;
    map<int32, string> m_id_resultName;
    FunctionalQueue *m_decompose_queue;                ///< 用户分解队列
    pthread_t m_decompose_thread;                      ///< 分解线程ID
    pthread_t m_save_result;                           ///< 用于自动保存结果的线程
    
    friend class Dispatcher;
    friend class SchedulerCommunicator;
    friend bool dcr::g_SchedulerNodeInitialize();
    friend bool dcr::g_SchedulerWork();
    friend bool dcr::g_SchedulerFinilize();
    friend class GroupInTable;
    friend class Strategy;
    friend class StrategyBlast;
    friend class StrategyType;
    
protected:
    TiXmlDocument type_doc;
};

///
/// \brief 用于描述计算任务的计算方法和归约方法,以及结果保存方法
///
class Job {
public:
    Job() {};
    
    virtual ~Job() {};
    
    virtual Result *computation(ComputationTask *task, OriginalTask *original_task) = 0;
    
    virtual void reduce(Result *compute_result, Result *reduce_result) = 0;
    
    virtual void saveResult(const vector<Result*>& result) = 0;

    ///
    /// \brief 每当收到一个结果包时会调用该方法，描述如何更新结果文件。
    ///
    virtual void updateResultFile(OriginalTask* original_task, int32 original_task_ID, Decomposer* decomposer, 
                                    Result *compute_result, Result *reduce_result, string path) = 0;

    ///
    /// \brief 分解器分解时调用的方法，描述如何更新结果文件。
    ///
    virtual void updateResultFileByDecomposer(OriginalTask* original_task, int32 original_task_ID, 
                                    Granularity* granularity, Decomposer* decomposer, string path, bool is_first_get) = 0;

    virtual void updateResultFileByTime(float time_rate) = 0;

    ///
    /// \brief 每当收到一个结果包时会调用该方法，描述如何更新日志文件。
    ///
    virtual void updateLogFile(Result *compute_result, Result *reduce_result) = 0;

    static  Mutex m_lock; 

    //static map<string, string> m_cache;

    ///
    /// \brief 根据任务信息来获取缓存内容。该接口用来供用户使用。
    ///
    string getCacheContext(ComputationTask *task, OriginalTask *original_task);

protected:
    ///
    /// \brief 根据任务信息来计算出缓存的ID，供缓存使用。
    ///
    virtual string getCacheID(ComputationTask *task, OriginalTask *original_task) = 0;

    ///
    /// \brief 根据任务信息来计算待缓存的内容，供缓存使用。
    ///
    virtual string getContextToBeCached(ComputationTask *task, OriginalTask *original_task) = 0;

private:
    ///
    /// \brief 判断cacheID是否在cache中。
    ///
    bool isCacheHit(string cacheID);

    ///
    /// \brief 将cacheID和cacheContext加入缓存中，该方法调用getContextTobeCache()。
    ///
    bool addCache(string cacheID, const string &cacheContext);

    ///
    /// \brief 根据cacheID来获得缓存内容。
    ///
    string getCache(string cacheID);


    
};


class XMLLog {
public:

    XMLLog() {};

    virtual ~XMLLog() { this->destroy(); };

    virtual bool init() {
        log_mutex = new Mutex();
        log_mutex->init();
        return true;
    }
    virtual bool destroy() {
        delete log_mutex;
        return true;
    }

    virtual bool createLOG();

    virtual bool add_StartTime();

    virtual bool add_ShutdownTime();

    virtual bool add_Tick();

    virtual bool add_ComputeNode(const vector<string>& ip_list);

    virtual bool add_DeleteComputeNode(const vector<string>& ip_list);
    
    virtual bool add_runJob(const Task& task);

    virtual bool add_finishJob(const Task& task);

    ///
    /// 根据时间来更新日志文件
    ///
    static void *updateByTime(void *arg);

    bool startWork();

    bool endWork();

protected:
    pthread_t m_update_by_time;                           ///< 用于自动保存结果的线程
    string log_file_path;
    Mutex *log_mutex;

};


DECLARE_CLASS_REGISTER_CENTER(dcr_task_registry,
                              Task);

#define REGISTER_TASK(task_name)                                            \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_task_registry,             \
                                             Task,                          \
                                             #task_name,                    \
                                             task_name)                     \

#define CREATE_TASK(task_name)                                              \
        CLASS_CREATE_OBJECT(dcr_task_registry, task_name)                   \



#define GET_TASK_SIZE(task_name)                                            \
        GET_OBJECT_SIZE(dcr_task_registry, task_name)                       \


DECLARE_CLASS_REGISTER_CENTER(dcr_xmllog_registry,
                              XMLLog);

#define REGISTER_XMLLOG(xmllog_name)                                          \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_xmllog_registry,             \
                                             XMLLog,                          \
                                             #xmllog_name,                    \
                                             xmllog_name)                     \

#define CREATE_XMLLOG(xmllog_name)                                            \
        CLASS_CREATE_OBJECT(dcr_xmllog_registry, xmllog_name)                 \



#define GET_XMLLOG_SIZE(xmllog_name)                                          \
        GET_OBJECT_SIZE(dcr_xmllog_registry, xmllog_name)                     \


///
/// \brief 用于声明基类的注册中心
///
DECLARE_CLASS_REGISTER_CENTER(dcr_decomposer_registry,
                              Decomposer);

DECLARE_CLASS_REGISTER_CENTER(dcr_original_task_registry,
                              OriginalTask);

DECLARE_CLASS_REGISTER_CENTER(dcr_computation_task_registry,
                              ComputationTask);

DECLARE_CLASS_REGISTER_CENTER(dcr_granulity_registry,
                              Granularity);

DECLARE_CLASS_REGISTER_CENTER(dcr_result_registry,
                              Result);

DECLARE_CLASS_REGISTER_CENTER(dcr_job_registry,
                              Job);

DECLARE_CLASS_REGISTER_CENTER(dcr_resource_registry,
                              Resource);

///
/// \brief 用于声明基类注册方法
///
#define REGISTER_DECOMPOSER(decomposer_name)                                            \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_decomposer_registry,                   \
                                             Decomposer,                                \
                                             #decomposer_name,                          \
                                             decomposer_name)                           \

#define REGISTER_ORIGINAL_TASK(original_task_name)                                      \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_original_task_registry,                \
                                             OriginalTask,                              \
                                             #original_task_name,                       \
                                             original_task_name)                        \


#define REGISTER_COMPUTATION_TASK(computation_task_name)                                \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_computation_task_registry,             \
                                             ComputationTask,                           \
                                             #computation_task_name,                    \
                                             computation_task_name)                     \

#define REGISTER_GRANULITY(granulity_name)                                              \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_granulity_registry,                    \
                                             Granularity,                               \
                                             #granulity_name,                           \
                                             granulity_name)                            \


#define REGISTER_RESULT(result_name)                                                    \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_result_registry,                       \
                                             Result,                                    \
                                             #result_name,                              \
                                             result_name)                               \


#define REGISTER_JOB(job_name)                                                          \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_job_registry,                          \
                                             Job,                                       \
                                             #job_name,                                 \
                                             job_name)                                  \


#define REGISTER_RESOURCE(resource_name)                                                \
        CLASS_OBJECT_CREATOR_REGISTER_ACTION(dcr_resource_registry,                     \
                                             Resource,                                  \
                                             #resource_name,                            \
                                             resource_name)                             \

///
/// \brief 用于声明创建基类的方法
///
#define CREATE_DECOMPOSER(decomposer_name)                                              \
        CLASS_CREATE_OBJECT(dcr_decomposer_registry, decomposer_name)                   \


#define CREATE_ORIGINAL_TASK(original_task_name)                                        \
        CLASS_CREATE_OBJECT(dcr_original_task_registry, original_task_name)             \


#define CREATE_COMPUTATION_TASK(computation_task_name)                                  \
        CLASS_CREATE_OBJECT(dcr_computation_task_registry, computation_task_name)       \


#define CREATE_GRANULITY(granuliry_name)                                                \
        CLASS_CREATE_OBJECT(dcr_granulity_registry, granuliry_name)                     \


#define CREATE_RESULT(result_name)                                                      \
        CLASS_CREATE_OBJECT(dcr_result_registry, result_name)                           \


#define CREATE_JOB(job_name)                                                            \
        CLASS_CREATE_OBJECT(dcr_job_registry, job_name)                                 \


#define CREATE_RESOURCE(resource_name)                                                  \
        CLASS_CREATE_OBJECT(dcr_resource_registry, resource_name)                       \


///
/// \brief 用于声明获取基类大小的方法
///
#define GET_DECOMPOSER_SIZE(decomposer_name)                                            \
        GET_OBJECT_SIZE(dcr_decomposer_registry, decomposer_name)                       \


#define GET_ORIGINAL_TASK_SIZE(original_task_name)                                      \
        GET_OBJECT_SIZE(dcr_original_task_registry, original_task_name)                 \


#define GET_COMPUTATION_TASK_SIZE(computation_task_name)                                \
        GET_OBJECT_SIZE(dcr_computation_task_registry, computation_task_name)           \


#define GET_GRANULITY_SIZE(granuliry_name)                                              \
        GET_OBJECT_SIZE(dcr_granulity_registry, granuliry_name)                         \


#define GET_RESULT_SIZE(result_name)                                                    \
        GET_OBJECT_SIZE(dcr_result_registry, result_name)                               \


#define GET_JOB_SIZE(job_name)                                                          \
        GET_OBJECT_SIZE(dcr_job_registry, job_name)                                     \


#define GET_RESOURCE_SIZE(resource_name)                                                \
        GET_OBJECT_SIZE(dcr_resource_registry, resource_name)                           \


#endif
