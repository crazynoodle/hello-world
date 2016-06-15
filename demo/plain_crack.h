#ifndef DCR_plain_crack_h
#define DCR_plain_crack_h

#include "../dcr/dcr_base_class.h"
#include "../util/common.h"
#include "../dcr/task_manager.h"
#include "../dcr/system_init.h"

#include "../cryption/include/sha256encryption.h"
#include "../cryption/include/aesencryption.h"
#include "../cryption/include/md5encryption.h"

#include <vector>

#define MAX_CIPHER_NUM  128
#define MAX_PLAIN_LEN   64

#define _MD5_ALG        0
#define _NTLM_ALG       1
#define _MS_CACHE_ALG   2
#define _MYSQL5_ALG     3
#define _DOMINO_ALG     4
#define _MD5_UNIX_ALG   5
#define _DES_UNIX_ALG   6
#define _SHA1_ALG       7
#define _QQ_ALG         8
#define _OFFICE2007_ALG 9
#define _OFFICE2010_ALG 10
#define _PDF_ALG        11
#define _WEP_ALG        12
#define _MEDIAWIKI_ALG  14
#define _HWP_ALG        15
#define _DISCUZ_ALG     16
#define _WORDPRESS_ALG  17
#define _PHPBB_ALG      18
#define _ORACLE7_10_ALG 19
#define _ORACLE11_ALG   20
#define _SSHA_ALG       21
#define _WPA_ALG        22
#define _LOTUS_ALG      23
#define _SINGLE_ALG     24
#define _SHA256_ALG     25
enum Finish_RE_Reason {RUN_ALL, Matched_Break, Admin_Break};
//atomic computation result
class ALGResult : public Result
{
public:
    bool init();
    ALGResult() {};
    ~ALGResult() {};
    struct timeval ts;
    struct timeval te;
    uint8  success_num;
    uint8 algorithm;
    uint8 cipher_len;
    char cipher[128][256]; 
    char salt[128][32];
    char ret_plain[MAX_CIPHER_NUM][MAX_PLAIN_LEN];
    int regulation_index;    
};
REGISTER_RESULT(ALGResult);

class ALGOriginalTask : public OriginalTask
{
public:
    bool readFromFileInit(TiXmlElement* const task_element);

    string createResultFile(TiXmlElement* const task_element, Task* task);
    ALGOriginalTask() {};
    
    ~ALGOriginalTask() {};
    
public:
    //char task_name[20];//任务名称
    uint8 algorithm;   //算法，在global.h中定义
    uint8 encodeType;  //编码类型0：UTF8，1：UCS2
    uint32 cipher_num;      //密文数目
    uint32 cipher_len;   //密文长度
    char cipher[128][256];  //密文最多128个
    uint8 salt_len;  //盐的长度
    char salt[128][32];//最多128个，最长32字节
    char re[32][256];
    uint8 indexfile_num;
    pthread_mutex_t mutex;//读写共享内存互斥量


};
REGISTER_ORIGINAL_TASK(ALGOriginalTask);

class ALGGranularity : public Granularity
{
public:
    bool readFromFileInit(TiXmlElement * const task_element);
    
    ALGGranularity() {};
    
    ~ALGGranularity() {};
    
public:
    uint32 granularity;
};
REGISTER_GRANULITY(ALGGranularity);

//atomic computation task
class ALGComputationTask : public ComputationTask
{
public:
    ALGComputationTask():encryptedPlainPath({0}),startBlockNo(0),blockNum(0),blockSize(0),plain_num(0),plain_len(0),secretKey({NULL}){};
    char encryptedPlainPath[256];//明文文件路径
    uint32 startBlockNo;//起始块号
    uint32 blockNum;    //读取的块数目
    uint64 blockSize;   //块的固定大小
    uint32 plain_num;    //需要读取的明文数目
    uint8 plain_len;    //明文长度
    char secretKey[512];     //秘钥
    int regulation_index;
    //uint64 plain_offset;//文件指针起始位置
};
REGISTER_COMPUTATION_TASK(ALGComputationTask);

class ALGDecomposer : public Decomposer
{
public:
    bool isFinishGetTask();
    
    ComputationTask *getTask(int32 original_task_ID, OriginalTask* original_task, Granularity* granularity, bool is_first_get);

    void updateResultFileByDecomposer(OriginalTask* original_task, int32 original_task_ID, Granularity* granularity, 
                                             string result_file_path, bool is_first_get);

    pair<int32, int32> getTypePrior(OriginalTask* original_task, Granularity* granularity, ComputationTask* compute_task, bool is_first_get);
    
    uint64 getPlainNum(char* plainfile_path,uint8 plain_len);

    void readIndexFile(char* indexfile_path,char* plainfile_path,uint64& totalPlainNum,uint8& plain_len,
                            uint32& totalBlockNum,uint32& blockSize,uint32& blockGran);

    ALGDecomposer(){};

    ~ALGDecomposer() {
        
        map<int32, int32*>::iterator it = m_re_decompose_count.begin();
        for (; it != m_re_decompose_count.end(); it++) {
            delete[] it->second;
            m_re_decompose_count.erase(it);
        }
    }
    
    uint8 indexfile_index;//索引下标
    uint8 indexfile_num;  //索引文件数目
    uint64 block_offset;  //索引文件内块的偏移量
    bool isUpdate[32];
    map<int32, int32*> m_re_decompose_count;
    
};
REGISTER_DECOMPOSER(ALGDecomposer);

class ALGJob : public Job
{
public:
    Result *computation(ComputationTask *task, OriginalTask *original_task);
    
    void reduce(Result *compute_result, Result *reduce_result);
    
    void saveResult(const vector<Result*>& final_result);

    ///
    /// \brief 每当收到一个结果包时会调用该方法，描述如何更新结果文件。
    ///
    void updateResultFile(OriginalTask* original_task, int32 original_task_ID, Decomposer* decomposer, 
                            Result *compute_result, Result *reduce_result, string path);

    void updateResultFileByDecomposer(OriginalTask* original_task, int32 original_task_ID, 
                                    Granularity* granularity, Decomposer* decomposer, string path, bool is_first_get);

    void updateResultFileByTime(float time_rate);

    ///
    /// \brief 每当收到一个结果包时会调用该方法，描述如何更新日志文件。
    ///
    void updateLogFile(Result *compute_result, Result *reduce_result);
    
    ALGJob() {};
    
    ~ALGJob() {};
protected:
    string getCacheID(ComputationTask *task, OriginalTask *original_task);

    string getContextToBeCached(ComputationTask *task, OriginalTask *original_task); 
};
REGISTER_JOB(ALGJob);


class PlainTask : public Task {
    
public:
    PlainTask() {};
    ~PlainTask() { this->destroy(); };

    bool destroy() {
    }

    string toString();

    int32 algorithm;
    int32 cipher_count;
    int32 finish_cipher_count;
    int32 left_cipher_count;
    //string cipher[5];

};
REGISTER_TASK(PlainTask);

class CrackXMLLog : public XMLLog {
public:
    CrackXMLLog() {};

    ~CrackXMLLog() {
        this->destroy();
    }

    bool init() {
        cout << "init child xml log " << endl;
        this->log_mutex = new Mutex();
        this->log_mutex->init();
        cout << "init CrackXMLLog!" << endl;
        return true;
    }
    
    bool destroy() {
        return true;
    }

    bool add_runJob(const Task& task);

    bool add_Regulation_Run(const string& regulation);

    bool add_Regulation_Finish(const string& regulation, int64 re_space, int32 finish_reason);

    bool add_Match(const int32 sys_task_ID, const int32 sys_target_ID, const string& user_task_ID, const int32 user_target_ID,
                    const int32 algorithm, const string& hashValue, const string& salt, const string& plain_text);

};
REGISTER_XMLLOG(CrackXMLLog);

#endif
