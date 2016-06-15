#include "../util/flags.h"

DEFINE_string(task_file_name, "", "任务文件名");
DEFINE_string(granulity_file_name, "", "任务文件名");
DEFINE_string(log_file_path, "", "日记文件路径");
DEFINE_bool(is_scheduler_node, true, "运行节点是否调度节点");
DEFINE_string(decomposer_name, "", "用户定义的分解器名称");
DEFINE_string(original_task_name, "", "用户定义的原始任务的名称");
DEFINE_string(granulity_name, "", "用户定义的粒度的名称");
DEFINE_string(computation_task_name, "", "用户定义的计算任务的名称");
DEFINE_string(result_name, "", "用户定义的计算结果的名称");
DEFINE_string(job_name, "", "用户定义的作业的名称");
DEFINE_int32(strategy, 0, "选择的调度策略");
DEFINE_string(node_type_file_name, "", "节点类型文件名");
DEFINE_string(task_name, "", "用户定义的作业类名称");
DEFINE_string(xmllog_name, "", "选择日志系统");
using namespace std;

//这里根据命令行的参数利用了反射来创建对象
namespace dcr {
    
    bool g_validateCommandLineFlags() {
        bool false_valid = true;
        return false_valid;
    }
    
    string g_getTaskFileName() {
        return fLS::FLAGS_task_file_name;
    }
    
    string g_getGranulityFileName() {
        return fLS::FLAGS_granulity_file_name;
    }
    
    bool g_isSchedulerNode() {
        return FLAGS_is_scheduler_node;
    }
    
    const char* g_workType() {
        return g_isSchedulerNode() ? "SchedulerNode" : "ComputeNode";
    }
    
    string g_getOriginalTaskName() {
        return fLS::FLAGS_original_task_name;
    }
    
    string g_getDecomposerName() {
        return fLS::FLAGS_decomposer_name;
    }
    
    string g_getComputationTaskName() {
        return fLS::FLAGS_computation_task_name;
    }
    
    string g_getGranulityName() {
        return fLS::FLAGS_granulity_name;
    }
    
    string g_getResultName() {
        return fLS::FLAGS_result_name;
    }
    
    string g_getJobName() {
        return fLS::FLAGS_job_name;
    }
    
    int32 g_getStrategyType() {
        return fLI::FLAGS_strategy;
    }
    
    string g_getNodeTypeFileName() {
        return fLS::FLAGS_node_type_file_name;
    }
    
    string g_getTaskName() {
        return fLS::FLAGS_task_name;
    }

    string g_getXMLLogName() {
        return fLS::FLAGS_xmllog_name;
    }

    string g_getHostName() {
        struct utsname buf;
        if (0 != uname(&buf)) {
            *buf.nodename = '\0';
        }
        return string(buf.nodename);
    }
    
    string g_getUserName() {
        const char *user_name = getenv("USER");
        return user_name != NULL? user_name : getenv("USERNAME");
    }
    
    
    string g_printCurrentTime(){
        time_t current_time = time(NULL);
        struct tm *broken_down_time;
        broken_down_time = localtime(&current_time);
        char temp[200];
        sprintf(temp, "%04dY.%02dM.%02dD-%02dH.%02dM.%02dS",
                1900 + broken_down_time->tm_year,
                1 + broken_down_time->tm_mon,
                broken_down_time->tm_mday, broken_down_time->tm_hour,
                broken_down_time->tm_min,  broken_down_time->tm_sec);
        return string(temp);
    }
    
    
    string g_logFilePrefix(){
        CHECK(!(FLAGS_log_file_path.empty()));
        char temp[200];
        string file_name_prefix;
        sprintf(temp, "%s-%s-of-%s.%s.%s.%u", FLAGS_log_file_path.c_str(),
                g_workType(), g_getHostName().c_str(), g_getUserName().c_str(),
                g_printCurrentTime().c_str(), getpid());
        return string(temp);
    }

    
    Decomposer* g_createDecomposer() {
        Decomposer* decomposer = NULL;
        if (g_isSchedulerNode()) {
            decomposer = CREATE_DECOMPOSER(fLS::FLAGS_decomposer_name);
            if (decomposer == NULL) {
                LOG(ERROR) << " Cannot create decomposer " << fLS::FLAGS_decomposer_name;
            }
        }
        return decomposer;
    }
    
    
    OriginalTask* g_createOriginalTask() {
        OriginalTask *original_task = NULL;
        original_task = CREATE_ORIGINAL_TASK(fLS::FLAGS_original_task_name);
        if (original_task == NULL) {
            LOG(ERROR) << " Cannot create original task " << fLS::FLAGS_original_task_name << endl;
        }
        return original_task;
    }
    
    
    ComputationTask* g_createComputationTask() {
        ComputationTask *computation_task = NULL;
        computation_task = CREATE_COMPUTATION_TASK(fLS::FLAGS_computation_task_name);
        if (computation_task == NULL) {
            LOG(ERROR) << " Cannot create computation task " << fLS::FLAGS_computation_task_name << endl;
        }
        return computation_task;
    }
    
    
    Granularity* g_createGranulity() {
        Granularity* granulity = NULL;
        granulity = CREATE_GRANULITY(fLS::FLAGS_granulity_name);
        if(granulity == NULL) {
            LOG(ERROR) << " Can not create granulity " << fLS::FLAGS_granulity_name << endl;
        }
        return granulity;
    }
    
    
    Result* g_createResult() {
        Result* result = NULL;
        result = CREATE_RESULT(fLS::FLAGS_result_name);
        if (result == NULL) {
            LOG(ERROR) << " Can not create result " << fLS::FLAGS_result_name << endl;
        }
        return result;
    }
    
    Job* g_createJob() {
        Job* job = NULL;
        job = CREATE_JOB(fLS::FLAGS_job_name);
        if(job == NULL) {
            LOG(ERROR) << " Can not create job" << fLS::FLAGS_job_name << endl;
        }
        return job;
    }
    
    Strategy* g_createStrategy() {
        Strategy* strategy = NULL;
        switch (g_getStrategyType()) {
            case 1:
                strategy = new Strategy;
                break;
            case 2:
                strategy = new StrategyBlast;
                break;
            case 3:
                strategy = new StrategyType;
                break;
            default:
                LOG(ERROR) << " strategy type error ";
                break;
        }
        return strategy;
    }

    Task* g_createTask() {
        Task* task = NULL;
        task = CREATE_TASK(fLS::FLAGS_task_name);
        if(task == NULL) {
            LOG(ERROR) << " Can not create task" << fLS::FLAGS_task_name << endl;
        }
        return task;
    }

    XMLLog* g_createXMLLog() {
        XMLLog* xmlLog = NULL;
        xmlLog = CREATE_XMLLOG(fLS::FLAGS_xmllog_name);
        if(xmlLog == NULL) {
            LOG(ERROR) << " Can not create xmlLog" << fLS::FLAGS_xmllog_name << endl;
        }
        return xmlLog;
    }
    
    int32 g_getOriginalTaskSize() {
        int32 original_object_size = GET_ORIGINAL_TASK_SIZE(fLS::FLAGS_original_task_name);
        if(original_object_size == -1) {
            LOG(ERROR) << " Can not get original task object size" << fLS::FLAGS_original_task_name << endl;
        }
        return original_object_size;
    }
    
    int32 g_getUdpTaskSize() {
        int32 computation_task_size = GET_COMPUTATION_TASK_SIZE(fLS::FLAGS_computation_task_name);
        computation_task_size += sizeof(UdpSendTaskHead);
        if (computation_task_size == -1) {
            LOG(ERROR) << " Can not get computation task size " << fLS::FLAGS_computation_task_name << endl;
        }
        return computation_task_size;
    }
    
    int32 g_getComputeTaskSize() {
        int32 computation_task_size = GET_COMPUTATION_TASK_SIZE(fLS::FLAGS_computation_task_name);
        if (computation_task_size == -1) {
            LOG(ERROR) << " Can not get computation task size " << fLS::FLAGS_computation_task_name << endl;
        }
        return computation_task_size;
    }
    
    int32 g_getGranulitySize() {
        int32 granulity_size = GET_GRANULITY_SIZE(fLS::FLAGS_granulity_name);
        if (granulity_size == -1) {
            LOG(ERROR) << " Can not get granulity size";
        }
        return granulity_size;
    }
    
    int32 g_getUdpResultSize() {
        int32 result_size = GET_RESULT_SIZE(fLS::FLAGS_result_name);
        if (result_size == -1) {
            LOG(ERROR) << " Can not get result size " << fLS::FLAGS_result_name << endl;
        }
        result_size += sizeof(UdpComputeResultHead);
        result_size += sizeof(SchedUdpRecvType);
        return result_size;
    }
    
    int32 g_getComputeResultSize() {
        int32 result_size = GET_RESULT_SIZE(fLS::FLAGS_result_name);
        if (result_size == -1) {
            LOG(ERROR) << " Can not get result size " << fLS::FLAGS_result_name << endl;
        }
        return result_size;
    }
    
    
}

