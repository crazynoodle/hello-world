#include "dcr_base_class.h"
#include "system_init.h"

#include "../util/common.h"
#include "../util/scoped_ptr.h"
#include "../util/logger.h"
#include "../util/flags.h"
#include "../core_data_structure/memory_queue.h"
#include "../core_data_structure/functional_queue.h"
#include "../dcr/strategy.h"
#include <sys/time.h>


IMPLEMENT_CLASS_REGISTER_CENTER(dcr_decomposer_registry,
                                Decomposer);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_original_task_registry,
                                OriginalTask);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_computation_task_registry,
                                ComputationTask);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_granulity_registry,
                                Granularity);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_result_registry,
                                Result);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_job_registry,
                                Job);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_resource_registry,
                                Resource);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_task_registry,
                                Task);

IMPLEMENT_CLASS_REGISTER_CENTER(dcr_xmllog_registry,
                                XMLLog);

Mutex Task::m_lock;

std::string itoa(uint32 num);

bool Task::destroy() {

}

string Task::toString() {

}


bool Decomposer::init() {
    //初始化线程锁
    this->m_mutex = new Mutex();
    this->m_mutex->init();  
    //初始化用户分解队列
    this->m_decompose_queue = new FunctionalQueue();
    if (!(this->m_decompose_queue->init(dcr::g_getComputationTaskQueue().get(), FQ_DECOMPOSER))) {
        LOG(ERROR) << "init decomposer_queue failed";
        return false;
    }
    
    return true;
}

int32 Decomposer::decomposeNewTask(Task *new_job_task) {
    
    string task_file_name = new_job_task->task_file_name;
    task_file_name = dcr::g_getConfig()->getTask_file_path() + task_file_name;
    string granulity_file_name = dcr::g_getGranulityFileName();
    TiXmlDocument task_doc(task_file_name.c_str());
    cout << "task name is " << task_file_name << endl;
    if (!(task_doc.LoadFile())) {
        LOG(ERROR) << " Load task file failed ";
        return -1;
    }
    TiXmlDocument gra_doc(granulity_file_name.c_str());
    if (!(gra_doc.LoadFile())) {
        LOG(ERROR) << " Load granulity file failed ";
        return -1;
    }
    TiXmlElement *task_element = task_doc.RootElement();
    TiXmlElement *gra_element = gra_doc.RootElement();

    OriginalTask *original_task = dcr::g_createOriginalTask();
    CHECK_NOTNULL(original_task);
    Granularity *granulity = dcr::g_createGranulity();
    CHECK_NOTNULL(granulity);
    Result *final_result = dcr::g_createResult();
    CHECK_NOTNULL(final_result);
        
    original_task->readFromFileInit(task_element);
    granulity->readFromFileInit(gra_element);
    final_result->init();
        
    int32 original_ID = this->insertOriginalTask(original_task);
    string result_path = original_task->createResultFile(task_element, new_job_task);
    this->m_id_resultName.insert(make_pair(original_ID, result_path));

    this->insertGranulity(granulity);
    this->insertFinalResult(final_result);

    this->m_result_num.insert(make_pair(original_ID, 0));
    this->m_decompose_num.insert(make_pair(original_ID, 0));
    dcr::g_getXMLLog()->add_runJob(*new_job_task);
    return original_ID;
}

bool Decomposer::destroy() {
    map<int32, OriginalTask*>::iterator ori_it = m_original_task_queue.begin();
    map<int32, Granularity*>::iterator gra_it = m_granulity_queue.begin();
    map<int32, Result*>::iterator re_it = m_final_result.begin();
    for (; ori_it != m_original_task_queue.end(); ori_it++, gra_it++, re_it++) {
        delete ori_it->second;
        delete gra_it->second;
        delete re_it->second;
        m_original_task_queue.erase(ori_it);
        m_granulity_queue.erase(gra_it);
        m_final_result.erase(re_it);
    }
    delete m_mutex;
    return true;
}

bool Decomposer::startWork() {
    // TODO 初始化Task的线程锁（为什么在这里）
    Task::m_lock.init();
    //创建线程进行分解
    int32 ret_val = pthread_create(&m_decompose_thread, NULL, decompose, this);
    if (ret_val == -1) {
        LOG(ERROR) << " Create decompose thread failed ";
        return false;
    }
    //创建线程保存结果
    ret_val = pthread_create(&m_save_result, NULL, save_result, this);
    if (ret_val == -1) {
        LOG(ERROR) << " Create save result thread failed ";
        return false;
    }
    
    return true;
}

bool Decomposer::endWork() {
    //记录每个原始任务分解成几个子任务的信息
    map<int32, int64>::iterator it = m_decompose_num.begin();
    for (; it != m_decompose_num.end(); it++) {
        LOG(INFO) << " original task: " << it->first
                  << " decompose " << it->second << " task. " << endl;
    }
    //记录每个原始任务得到的结果的信息
    it = m_result_num.begin();
    for (; it != m_result_num.end(); it++) {
        LOG(INFO) << " original task: " << it->first
                  << " recieve " << it->second << " result. " << endl;
    }
    //终止分解线程
    int32 thread_exit_flag = pthread_kill(m_decompose_thread, 0);
    if(thread_exit_flag == ESRCH) {
        return true;
    } else if(thread_exit_flag == EINVAL) {
        LOG(ERROR) << " illegal signal! " << endl;
        return false;
    } else {
        int32 err = pthread_cancel(m_decompose_thread);
        if (err != 0) {
            LOG(ERROR) << " cancel decompose thread failed! ";
            return false;
        }
        
    }
    //终止保存结果线程
    int32 err = pthread_cancel(m_save_result);
    if (err != 0) {
        LOG(ERROR) << " cancel save result failed! ";
        return false;
    }
    
    return true;
}

OriginalTask* Decomposer::getOriginalTaskByID(int32 ID) {
    this->m_mutex->Lock();
    CHECK_GE(ID, 0);
    int64 original_task_queue_size = this->m_original_task_queue.size();
    CHECK_LE(ID, original_task_queue_size);
    map<int32, OriginalTask*>::const_iterator it = this->m_original_task_queue.find(ID);
    if (it != this->m_original_task_queue.end()) {
        this->m_mutex->unLock();
        return it->second;
    } else {
        LOG(ERROR) << "Can not find Original task " << ID;
        this->m_mutex->unLock();
        return NULL;
    }
}

Granularity* Decomposer::getGranulityByID(int32 ID) {
    this->m_mutex->Lock();
    CHECK_GE(ID, 0);
    int64 granulity_queue_size = this->m_original_task_queue.size();
    CHECK_LE(ID, granulity_queue_size);
    map<int32, Granularity*>::const_iterator it = this->m_granulity_queue.find(ID);
    if (it != this->m_granulity_queue.end()) {
        this->m_mutex->unLock();
        return it->second;
    } else {
        LOG(ERROR) << "Can not find Granulity " << ID;
        this->m_mutex->unLock();
        return NULL;
    }
}

Result* Decomposer::getFinalResultByID(int32 ID) {
    this->m_mutex->Lock();
    CHECK_GE(ID, 0);
    int64 result_queue_size = this->m_final_result.size();
    CHECK_LE(ID, result_queue_size);
    map<int32, Result*>::const_iterator it = this->m_final_result.find(ID);
    if (it != this->m_final_result.end()) {
        this->m_mutex->unLock();
        return it->second;
    } else {
        LOG(ERROR) << " Can not find FinalResult" << ID;
        this->m_mutex->unLock();
        return NULL;
    }
}

int64 Decomposer::getDecomposeNumByID(int32 ID) {
    this->m_mutex->Lock();
    CHECK_GE(ID, 0);
    int64 size = this->m_decompose_num.size();
    CHECK_LE(ID, size);
    map<int32, int64>::const_iterator it = this->m_decompose_num.find(ID);
    if (it != this->m_decompose_num.end()) {
        this->m_mutex->unLock();
        return it->second;
    } else {
        LOG(ERROR) << " Can not find decompose num! " << ID;
        this->m_mutex->unLock();
        return -1;
    }
}

int64 Decomposer::getResultNumByID(int32 ID) {
    this->m_mutex->Lock();
    CHECK_GE(ID, 0);
    int64 size = this->m_decompose_num.size();
    CHECK_LE(ID, size);
    map<int32, int64>::const_iterator it = this->m_decompose_num.find(ID);
    if (it != this->m_decompose_num.end()) {
        this->m_mutex->unLock();
        return it->second;
    } else {
        LOG(ERROR) << " Can not find result num! " << ID;
        this->m_mutex->unLock();
        return -1;
    }
}

FunctionalQueue* Decomposer::getDecomposeQueue() {
    return this->m_decompose_queue;
}

int32 Decomposer::insertOriginalTask(OriginalTask *original_task) {
    CHECK_NOTNULL(original_task);
    this->m_mutex->Lock();
    int64 original_task_queue_size = this->m_original_task_queue.size();
    int64 new_ID = original_task_queue_size + 1;
    //int32 global_ID = dcr::g_getTaskManager()->get_global_task_ID();
    /*if (new_ID != global_ID) {
        LOG(ERROR) << " new id is " << new_ID << " and global id is " << global_ID;
        this->m_mutex->unLock();
        return new_ID;
    }*/
    this->m_original_task_queue.insert(make_pair(new_ID, original_task));
    this->m_mutex->unLock();
    return new_ID;
}

bool Decomposer::insertGranulity(Granularity *granulity) {
    CHECK_NOTNULL(granulity);
    this->m_mutex->Lock();
    int64 granulity_queue_size = this->m_granulity_queue.size();
    int64 new_ID = granulity_queue_size + 1;
    //int32 global_ID = dcr::g_getTaskManager()->get_global_task_ID();
    //LOG(ERROR) << " new id is " << new_ID;
    /*
    if (new_ID != global_ID) {
        LOG(ERROR) << " new id is " << new_ID << " and global id is " << global_ID;
        this->m_mutex->unLock();
        return false;
    }*/
    this->m_granulity_queue.insert(make_pair(new_ID, granulity));
    this->m_mutex->unLock();
    return true;
}

bool Decomposer::insertFinalResult(Result *final_result) {
    CHECK_NOTNULL(final_result);
    this->m_mutex->Lock();
    int64 result_queue_size = this->m_final_result.size();
    int64 new_ID = result_queue_size + 1;
    /*
    int32 global_ID = dcr::g_getTaskManager()->get_global_task_ID();
    if (new_ID != global_ID) {
        LOG(ERROR) << " new id is " << new_ID << " and global id is " << global_ID;
        this->m_mutex->unLock();
        return false;
    }*/
    this->m_final_result.insert(make_pair(new_ID, final_result));
    this->m_mutex->unLock();
    return true;
}

int32 Decomposer::computeDecomposeSpeed() {
    int32 node_num = (int32)dcr::g_getComputeNodeTable()->active_ip_list.size() + 1;
    return (dcr::g_getConfig()->getTaskQueuePerNode() * node_num);
}

void* Decomposer::decompose(void* arg) {
    
    Decomposer* ptr = static_cast<Decomposer*>(arg);
    
    dcr::g_getStrategy()->decomposeStrategy(ptr);
    
    pthread_testcancel();
    LOG(INFO) << " exit decompose thread! ";
    return NULL;
}

void* Decomposer::save_result(void *arg) {
    
    Decomposer* ptr = static_cast<Decomposer*>(arg);
        
    vector<Result*> result;
    while (true) {
        int64 result_num = ptr->m_final_result.size();
        if (result_num <= 0) {
            sleep(10);
            continue;
        }
        result.clear();

        for (int32 i = 0; i < result_num; i++) {
            result.push_back(ptr->m_final_result[i+1]);
        }

        dcr::g_getJob()->saveResult(result);
        sleep(10);
        pthread_testcancel();
    }
    
    dcr::g_getJob()->saveResult(result);
    
    LOG(INFO) << " exit save result thread! ";
    return NULL;
}




string Job::getCacheContext(ComputationTask *task, OriginalTask *original_task) {
    Job::m_lock.Lock();
    string cacheID = getCacheID(task, original_task);
    map<string, string>::iterator it = Caculator::m_cache.find(cacheID);
    //LOG(INFO) << " cache size is " << Caculator::cache_sequence.size();
    if(it != Caculator::m_cache.end()) {
        //将访问的元素放在cache_sequence的链表头,实现LRU
        list<string>::iterator list_iter = Caculator::cache_sequence.begin();
        for (; list_iter != Caculator::cache_sequence.end() ; list_iter++) {
            if (*list_iter == cacheID) {
                list_iter = Caculator::cache_sequence.erase(list_iter);
                Caculator::cache_sequence.push_front(cacheID);
                break;
            }
        }
        //LOG(INFO) << " read content from cache.";
        string context = it->second;
        Job::m_lock.unLock();
        return context;
    }
    else {
        string context = getContextToBeCached(task, original_task);
        if(!addCache(cacheID, context)) {
            LOG(ERROR) << " add cache error ";
        } else {
            //将元素插入表头，如果缓存已满，则将表尾元素删除
            Caculator::cache_sequence.push_front(cacheID);
            if (Caculator::cache_sequence.size() > MAX_CACHE_SIZE) {
                string delete_ID = Caculator::cache_sequence.back();
                Caculator::cache_sequence.pop_back();
                map<string, string>::iterator delete_it = Caculator::m_cache.find(delete_ID);
                if (delete_it != Caculator::m_cache.end()) {
                    Caculator::m_cache.erase(delete_it);
                }
            }
        }
        Job::m_lock.unLock();
        return context;
    }
    
}

bool Job::isCacheHit(string cacheID) {
    map<string, string>::iterator it = Caculator::m_cache.find(cacheID);
    if(it == Caculator::m_cache.end())
        return false;
    return true;
}

bool Job::addCache(string cacheID, const string &cacheContext) {
    
    Caculator::m_cache.insert(pair<string, string>(cacheID, cacheContext));
    //Caculator::m_cache[cacheID] = cacheContext;
    return true;
}

string Job::getCache(string cacheID) {
    return NULL;
}


//如果系统中存在日志文件，则直接返回
//否则创建日志文件
bool XMLLog::createLOG() {

    this->log_file_path = "DCR.LOG.xml";
    TiXmlDocument mydoc(log_file_path.c_str());
    if(!mydoc.LoadFile()) {
        TiXmlElement *pLogNode = new TiXmlElement("LOG");
        mydoc.LinkEndChild(pLogNode);

        TiXmlElement *pHeadNode = new TiXmlElement("Head");
        pLogNode->LinkEndChild(pHeadNode);
        
        TiXmlElement *pNode = new TiXmlElement("Version");
        pHeadNode->LinkEndChild(pNode);
        TiXmlText *pText = new TiXmlText(dcr::g_getConfig()->getVersion().c_str());
        pNode->LinkEndChild(pText);
        
        pNode = new TiXmlElement("Type");
        pHeadNode->LinkEndChild(pNode);
        pText = new TiXmlText("LOG");
        pNode->LinkEndChild(pText);
        
        pNode = new TiXmlElement("Machine");
        pHeadNode->LinkEndChild(pNode);
        pText = new TiXmlText(dcr::g_getConfig()->getMachine().c_str());
        pNode->LinkEndChild(pText);

        pNode = new TiXmlElement("Tick_count");
        pHeadNode->LinkEndChild(pNode);
        pText = new TiXmlText("0");
        pNode->LinkEndChild(pText);

        pNode = new TiXmlElement("Last_Tick_Time");
        pHeadNode->LinkEndChild(pNode);
        pText = new TiXmlText("");
        pNode->LinkEndChild(pText);

        pNode = new TiXmlElement("Head_Check_Tick");
        pHeadNode->LinkEndChild(pNode);
        pText = new TiXmlText("");
        pNode->LinkEndChild(pText);

        mydoc.SaveFile();
    }
    return true;
}

bool XMLLog::add_StartTime() {
    TiXmlDocument mydoc;
    TiXmlElement *rootEle = new TiXmlElement("Start");
    mydoc.LinkEndChild(rootEle);
    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    time_t tt = now_time.tv_sec;
    tm *temp = localtime(&tt);
    char time_str[32];
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900,
             temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);

    TiXmlElement *time_ele = new TiXmlElement("Time");
    rootEle->LinkEndChild(time_ele);
    TiXmlText *time_text = new TiXmlText(time_str);
    time_ele->LinkEndChild(time_text);

    this->log_mutex->Lock();
    int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    mydoc.Accept(&log_printer);
    //将标记写在文件的末尾，定位到</LOG>之前。
    lseek(fd, -7, SEEK_END);
    string space = "\n";
    write(fd, space.c_str(), space.length());
    write(fd, log_printer.CStr(), log_printer.Size());
    //写入被覆盖的</LOG>。
    string logtag = "</LOG>";
    write(fd, logtag.c_str(), logtag.length());
    close(fd);

    this->log_mutex->unLock();
    return true;
}

bool XMLLog::add_ShutdownTime() {
    TiXmlDocument mydoc;
    TiXmlElement *rootEle = new TiXmlElement("Shutdown");
    mydoc.LinkEndChild(rootEle);
    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    time_t tt = now_time.tv_sec;
    tm *temp = localtime(&tt);
    char time_str[32];
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900,
             temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);

    TiXmlElement *time_ele = new TiXmlElement("Time");
    rootEle->LinkEndChild(time_ele);
    TiXmlText *time_text = new TiXmlText(time_str);
    time_ele->LinkEndChild(time_text);

    this->log_mutex->Lock();

    int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    mydoc.Accept(&log_printer);

    lseek(fd, -7, SEEK_END);
    string space = "\n";
    write(fd, space.c_str(), space.length());
    write(fd, log_printer.CStr(), log_printer.Size());
    string logtag = "</LOG>";
    write(fd, logtag.c_str(), logtag.length());
    close(fd);

    this->log_mutex->unLock();
    return true;
}

bool XMLLog::add_Tick() {
    return true;
}

bool XMLLog::add_ComputeNode(const vector<string>& ip_list) {

    TiXmlDocument mydoc;
    TiXmlElement *rootEle = new TiXmlElement("Add_CN");
    mydoc.LinkEndChild(rootEle);

    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    time_t tt = now_time.tv_sec;
    tm *temp = localtime(&tt);
    char time_str[32];
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900,
             temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);

    TiXmlElement *time_ele = new TiXmlElement("Time");
    TiXmlText *time_text = new TiXmlText(time_str);
    time_ele->LinkEndChild(time_text);
    rootEle->LinkEndChild(time_ele);

    int32 ip_num = ip_list.size();
    TiXmlElement *add_num_ele = new TiXmlElement("Add_Num");
    TiXmlText *add_num_text = new TiXmlText(itoa(ip_num).c_str());
    add_num_ele->LinkEndChild(add_num_text);
    rootEle->LinkEndChild(add_num_ele);

    vector<string>::const_iterator iter;
    for(iter = ip_list.begin(); iter != ip_list.end(); iter++){  
        TiXmlElement *cn_ip_ele = new TiXmlElement("CN_IP");
        TiXmlText *cn_ip_text = new TiXmlText((*iter).c_str());
        cn_ip_ele->LinkEndChild(cn_ip_text);
        rootEle->LinkEndChild(cn_ip_ele); 
    } 

    int32 group_num = dcr::g_getComputeNodeTable()->getGroupNum();
    int32 total_cn_num = 0;
    for (int32 i = 0; i < group_num; i ++) {
        GroupInTable *group_in_table = dcr::g_getComputeNodeTable()->getGroup(i);
        FunctionalQueue *active_compute_node = group_in_table->getActiveComputeNode();
        total_cn_num += active_compute_node->getLen();
    }

    TiXmlElement *cn_nodes_ele = new TiXmlElement("CN_NODES");
    TiXmlText *cn_nodes_text = new TiXmlText(itoa(total_cn_num).c_str());
    cn_nodes_ele->LinkEndChild(cn_nodes_text);
    rootEle->LinkEndChild(cn_nodes_ele);

    this->log_mutex->Lock();

    int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    mydoc.Accept(&log_printer);

    lseek(fd, -7, SEEK_END);
    string space = "\n";
    write(fd, space.c_str(), space.length());
    write(fd, log_printer.CStr(), log_printer.Size());
    string logtag = "</LOG>";
    write(fd, logtag.c_str(), logtag.length());
    close(fd);

    this->log_mutex->unLock();

    return true;
}

bool XMLLog::add_DeleteComputeNode(const vector<string>& ip_list) {

    TiXmlDocument mydoc;
    TiXmlElement *rootEle = new TiXmlElement("Del_CN");
    mydoc.LinkEndChild(rootEle);

    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    time_t tt = now_time.tv_sec;
    tm *temp = localtime(&tt);
    char time_str[32];
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900,
             temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);

    TiXmlElement *time_ele = new TiXmlElement("Time");
    TiXmlText *time_text = new TiXmlText(time_str);
    time_ele->LinkEndChild(time_text);
    rootEle->LinkEndChild(time_ele);

    int32 ip_num = ip_list.size();
    TiXmlElement *add_num_ele = new TiXmlElement("Del_Num");
    TiXmlText *add_num_text = new TiXmlText(itoa(ip_num).c_str());
    add_num_ele->LinkEndChild(add_num_text);
    rootEle->LinkEndChild(add_num_ele);

    vector<string>::const_iterator iter;
    for(iter = ip_list.begin(); iter != ip_list.end(); iter++){  
        TiXmlElement *cn_ip_ele = new TiXmlElement("CN_IP");
        TiXmlText *cn_ip_text = new TiXmlText((*iter).c_str());
        cn_ip_ele->LinkEndChild(cn_ip_text);
        rootEle->LinkEndChild(cn_ip_ele); 
    } 

    int32 group_num = dcr::g_getComputeNodeTable()->getGroupNum();
    int32 total_cn_num = 0;
    for (int32 i = 0; i < group_num; i ++) {
        GroupInTable *group_in_table = dcr::g_getComputeNodeTable()->getGroup(i);
        FunctionalQueue *active_compute_node = group_in_table->getActiveComputeNode();
        total_cn_num += active_compute_node->getLen();
    }

    TiXmlElement *cn_nodes_ele = new TiXmlElement("CN_NODES");
    TiXmlText *cn_nodes_text = new TiXmlText(itoa(total_cn_num).c_str());
    cn_nodes_ele->LinkEndChild(cn_nodes_text);
    rootEle->LinkEndChild(cn_nodes_ele);

    this->log_mutex->Lock();

    int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    mydoc.Accept(&log_printer);

    lseek(fd, -7, SEEK_END);
    string space = "\n";
    write(fd, space.c_str(), space.length());
    write(fd, log_printer.CStr(), log_printer.Size());
    string logtag = "</LOG>";
    write(fd, logtag.c_str(), logtag.length());
    close(fd);

    this->log_mutex->unLock();

    return true;
}
    
bool XMLLog::add_runJob(const Task& task) {
    TiXmlDocument mydoc;
    TiXmlElement *rootEle = new TiXmlElement("RUN_GPR");
    mydoc.LinkEndChild(rootEle);

    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    time_t tt = now_time.tv_sec;
    tm *temp = localtime(&tt);
    char time_str[32];
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900,
             temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);

    TiXmlElement *time_ele = new TiXmlElement("Time");
    TiXmlText *time_text = new TiXmlText(time_str);
    time_ele->LinkEndChild(time_text);
    rootEle->LinkEndChild(time_ele);


    TiXmlElement *task_id_ele = new TiXmlElement("Sys_Task_ID");
    TiXmlText *task_id_text = new TiXmlText(itoa(task.task_id).c_str());
    task_id_ele->LinkEndChild(task_id_text);
    rootEle->LinkEndChild(task_id_ele);

    this->log_mutex->Lock();

    int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    mydoc.Accept(&log_printer);

    lseek(fd, -7, SEEK_END);
    string space = "\n";
    write(fd, space.c_str(), space.length());
    write(fd, log_printer.CStr(), log_printer.Size());
    string logtag = "</LOG>";
    write(fd, logtag.c_str(), logtag.length());
    close(fd);

    this->log_mutex->unLock();
    
    return true;    
}

bool XMLLog::add_finishJob(const Task& task) {
    TiXmlDocument mydoc;
    TiXmlElement *rootEle = new TiXmlElement("FINISH_GPR");
    mydoc.LinkEndChild(rootEle);

    struct timeval now_time;
    gettimeofday(&now_time, NULL);
    time_t tt = now_time.tv_sec;
    tm *temp = localtime(&tt);
    char time_str[32];
    sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900,
             temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);

    TiXmlElement *time_ele = new TiXmlElement("Finish_Time");
    TiXmlText *time_text = new TiXmlText(time_str);
    time_ele->LinkEndChild(time_text);
    rootEle->LinkEndChild(time_ele);


    TiXmlElement *task_id_ele = new TiXmlElement("Sys_Task_ID");
    TiXmlText *task_id_text = new TiXmlText(itoa(task.task_id).c_str());
    task_id_ele->LinkEndChild(task_id_text);
    rootEle->LinkEndChild(task_id_ele);

    this->log_mutex->Lock();

    int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    mydoc.Accept(&log_printer);

    lseek(fd, -7, SEEK_END);
    string space = "\n";
    write(fd, space.c_str(), space.length());
    write(fd, log_printer.CStr(), log_printer.Size());
    string logtag = "</LOG>";
    write(fd, logtag.c_str(), logtag.length());
    close(fd);

    this->log_mutex->unLock();
    
    return true;    

}

void* XMLLog::updateByTime(void *arg) {
    LOG(INFO) << " start update log thread! ";
    int32 sleep_time = 10;
    while (true) {
        system("echo 1 > /proc/sys/vm/drop_caches");
        sleep(sleep_time);
        dcr::g_getXMLLog()->add_Tick();

        float rate = (float)sleep_time / (60 * 60);
        dcr::g_getJob()->updateResultFileByTime(rate);

        pthread_testcancel();
    }
    
    LOG(INFO) << " exit update log thread! ";
    return NULL;
}

bool XMLLog::startWork() {
    int32 ret_val = pthread_create(&m_update_by_time, NULL, updateByTime, this);
    if (ret_val == -1) {
        LOG(ERROR) << " Create xmlLog thread failed ";
        return false;
    }
    return true;
}

bool XMLLog::endWork() {
    int32 ret_val = pthread_cancel(m_update_by_time);
    if (ret_val != 0) {
        LOG(ERROR) << "cancel xmlLog thread failed";
        return false;
    }
    return true;
}