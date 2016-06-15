#include "../dcr/system_init.h"

namespace dcr {
    
    scoped_ptr<MemoryQueue>& g_getComputationTaskQueue() {
        static scoped_ptr<MemoryQueue> task_queue(new MemoryQueue());
        return task_queue;
    }
    
    scoped_ptr<MemoryQueue>& g_getLoginQueue() {
        static scoped_ptr<MemoryQueue> login_queue(new MemoryQueue());
        return login_queue;
    }
    
    scoped_ptr<MemoryQueue>& g_getResultQueue() {
        static scoped_ptr<MemoryQueue> result_queue(new MemoryQueue());
        return result_queue;
    }
    
    scoped_ptr<ComputeNodeTable>& g_getComputeNodeTable() {
        static scoped_ptr<ComputeNodeTable> compute_node_table(new ComputeNodeTable());
        return compute_node_table;
    }
    
    scoped_ptr<Decomposer>& g_getDecomposer() {
        static scoped_ptr<Decomposer> decomposer(g_createDecomposer());
        return decomposer;
    }
    
    scoped_ptr<SchedulerCommunicator>& g_getSchedulerCommunicator() {
        static scoped_ptr<SchedulerCommunicator> scheduler_communicator(new SchedulerCommunicator());
        return scheduler_communicator;
    }
    
    scoped_ptr<Dispatcher>& g_getDispatcher() {
        static scoped_ptr<Dispatcher> dispatcher(new Dispatcher());
        return dispatcher;
    }
    
    scoped_ptr<Job>& g_getJob() {
        static scoped_ptr<Job> job(g_createJob());
        return job;
    }
    
    scoped_ptr<SystemConfig>& g_getConfig() {
        static scoped_ptr<SystemConfig> config(new SystemConfig());
        return config;
    }
    
    scoped_ptr<Strategy>& g_getStrategy() {
        static scoped_ptr<Strategy> strategy(g_createStrategy());
        return strategy;
    }
    
    scoped_ptr<TaskManager>& g_getTaskManager() {
        static scoped_ptr<TaskManager> taskManager(new TaskManager());
        return taskManager;
    }

    scoped_ptr<XMLLog>& g_getXMLLog() {
        static scoped_ptr<XMLLog> xmlLog(g_createXMLLog());
        return xmlLog;
    }
    
    scoped_ptr<Asynchronous>& g_getAsynchronous() {
        static scoped_ptr<Asynchronous> asyn(new Asynchronous());
        return asyn;
    }

    bool g_SchedulerNodeInitialize(){
        //验证参数正确性
        if (!g_validateCommandLineFlags()) {
            LOG(ERROR) << "Failed validating command line";
            return false;
        }
        //初始化日志系统
        string log_file_name_prefix_info = g_logFilePrefix();
        string log_file_name_prefix_warn = g_logFilePrefix();
        string log_file_name_prefix_error = g_logFilePrefix();
        initLogger(log_file_name_prefix_info.append(".INFO"),
                   log_file_name_prefix_warn.append(".WARN"),
                   log_file_name_prefix_error.append(".ERROR"));
        //读取配置文件
        if(!(g_getConfig()->init())) {
            LOG(ERROR) << " Can not init Config! ";
            return false;
        }
        //初始化计算任务队列（内存池）
        if (!(g_getComputationTaskQueue()->init(MQ_TASK, g_getUdpTaskSize(), g_getConfig()->getTaskQueueLen()))) {
            LOG(ERROR) << " Can not init task queue! ";
            return false;
        }
        //初始化结果队列（内存池）
        if (!(g_getResultQueue()->init(MQ_RESULT, g_getUdpResultSize(), g_getConfig()->getResultQueueLen()))) {
            LOG(ERROR) << " Can not init result queue! ";
            return false;
        }
        //初始化登录队列（内存池）
        if (!(g_getLoginQueue()->init(MQ_LOGIN, sizeof(LoginPacket), g_getConfig()->getLoginQueueLen()))) {
            LOG(ERROR) << " Can not init login queue! ";
            return false;
        }
        //初始化计算节点表（内存池）
        if (!(g_getComputeNodeTable()->init(g_getConfig()->getNodeGroup(), g_getConfig()->getNodePerGroup()))) {
            LOG(ERROR) << " Can not init computeNode table! ";
            return false;
        }
        //初始化分解器
        if (!(g_getDecomposer()->init())) {
            LOG(ERROR) << " Can not init decomposer! ";
            return false;
        }
        //初始化调度节点和计算节点的通信模块
        if (!(g_getSchedulerCommunicator()->init())) {
            LOG(ERROR) << " Can not init scheduler_communicator";
            return false;
        }
        //初始化计算任务分发器
        if(!(g_getDispatcher()->init())) {
            LOG(ERROR) << " Can not init dispatcher! ";
            return false;
        }
        // TODO: 下面的几个初始化有待分析
        //初始化策略
        if (!(g_getStrategy()->init())) {
            LOG(ERROR) << " Can not init strategy! ";
            return false;
        }

        if (!(g_getTaskManager()->init())) {
            LOG(ERROR) << " Can not init taskManager! ";
            return false;
        }

        if (!(dcr::g_getXMLLog()->init())) {
            LOG(ERROR) << " Can not init XMLLOG! ";
            return false;
        }
        
        if (!(g_getAsynchronous()->setupClient("127.0.0.1", UDP_ASYN_PORT))) {
            LOG(ERROR) << " Can not init Asynchronous! ";
            return false;
        }
        return true;
    }
    
    
    bool g_SchedulerWork(){
        //网络模块任务开启
        if (!(g_getSchedulerCommunicator().get()->startWork())) {
            LOG(ERROR) << "start communicator work failed";
            return false;
        }
        //分解模块任务开启
        if (!(g_getDecomposer().get()->startWork())) {
            LOG(ERROR) << "start decomposer work failed";
            return false;
        }
        //分发模块任务开启
        if (!(g_getDispatcher().get()->startWork(g_getConfig()->getNodeGroup()))) {
            LOG(ERROR) << "start dispatch work failed";
            return false;
        }

        if (!(g_getXMLLog().get()->startWork())) {
            LOG(ERROR) << "start xml work failed";
            return false;
        }
        
        return true;
        
    }
    
    
    bool g_SchedulerFinilize() {
        //结束顺序与开启顺序刚好相反
        //等待分发线程结束
        g_getDispatcher()->endWork(g_getConfig()->getNodeGroup());
        //等待分解模块结束
        g_getDecomposer()->endWork();
        //等待网络模块结束
        g_getSchedulerCommunicator()->endWork();
        LOG(INFO) << " all work has been finished! " << endl;
        return true;
    }
    
    
    scoped_ptr<MemoryQueue>& g_getTaskQueueOfComputeNode() {
        static scoped_ptr<MemoryQueue> task_queue(new MemoryQueue());
        return task_queue;
    }
    
    scoped_ptr<ComputeCommunicator>& g_getComputeCommunicator() {
        static scoped_ptr<ComputeCommunicator> compute_node_communicator(new ComputeCommunicator());
        return compute_node_communicator;
    }
    
    scoped_ptr<Caculator>& g_getCaculator() {
        static scoped_ptr<Caculator> caculator(new Caculator());
        return caculator;
    }
    
    bool g_ComputeNodeInitialize(){
        //验证参数正确性
        if (!g_validateCommandLineFlags()) {
            LOG(ERROR) << "Failed validating command line";
            return false;
        }
        //初始化日志记录系统
        string log_file_name_prefix_info = g_logFilePrefix();
        string log_file_name_prefix_warn = g_logFilePrefix();
        string log_file_name_prefix_error = g_logFilePrefix();
        initLogger(log_file_name_prefix_info.append(".INFO"),
                   log_file_name_prefix_warn.append(".WARN"),
                   log_file_name_prefix_error.append(".ERROR"));
        //读取配置
        if (!(g_getConfig()->init())) {
            LOG(ERROR) << " init config error! ";
            return false;
        }
        //初始化计算节点任务队列（内存池）
        if (!(g_getTaskQueueOfComputeNode()->init(MQ_TASK, g_getUdpTaskSize(),
                                                  g_getConfig()->getTaskQueuePerNode()))) {
            LOG(ERROR) <<" init task queue failed ";
            return false;
        }
        //初始化计算节点的通信模块
        if (!(g_getComputeCommunicator()->init())) {
            LOG(ERROR) << " init compute communicator fail ";
            return false;
        }
        //初始化计算器
        if (!(g_getCaculator()->init())) {
            LOG(ERROR) << " init calculator failed ";
            return false;
        }
        
        return true;
    }
    
    bool g_ComputeWork(){
        //网络模块任务开启
        if (!(g_getComputeCommunicator()->startWork())) {
            LOG(ERROR) << " compute node communicator start work failed ";
            return false;
        }
        //计算模块任务开启
        if (!(g_getCaculator()->startWork())) {
            LOG(ERROR) << " compute node caculator start work failed ";
            return false;
        }
        
        return true;
    }
    
    bool g_ComputeFinilize() {
        //等待计算模块结束
        g_getCaculator()->endWork();
        //等待网络模块结束
        g_getComputeCommunicator()->endWork();
        return true;
    }
    
}

