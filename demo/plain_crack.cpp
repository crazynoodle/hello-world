#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <cmath>
#include <vector>
#include <sys/utsname.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sstream>
#include <sys/wait.h>
#include <memory>
#include <sys/types.h>
#include <fstream>
#include <string>
#include <sys/time.h>

using namespace std;

#include "plain_crack.h"

std::string itoa(uint32 num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

bool ALGResult::init()
{
    ts={0};
	te={0};
	success_num=0;
	algorithm=0;
	cipher_len=0;
	memset(cipher,NULL,sizeof(cipher)); 
	memset(cipher,NULL,sizeof(salt));
	memset(ret_plain,NULL,sizeof(ret_plain));
    return true;
}
int32 algorithm_ID = 0;
bool ALGOriginalTask::readFromFileInit(TiXmlElement * const task_element) {
    if (task_element == NULL) {
        LOG(ERROR) << "task_element is NULL";
        return false;
    }
    memset(cipher, NULL, 128 * 256);
    memset(salt, NULL, 128 * 32);
    memset(re, NULL, MAX_CIPHER_NUM * MAX_PLAIN_LEN);
    //init Head Tag
    TiXmlElement *head = task_element->FirstChildElement("Head");
    TiXmlElement *algorithm = head->FirstChildElement("Algorithm");
	this->algorithm = atoi(algorithm->GetText());
	algorithm_ID = this->algorithm;
	this->encodeType = 0;
	if(this->algorithm==_NTLM_ALG||this->algorithm==_NTLM_ALG==_MS_CACHE_ALG||this->algorithm==_OFFICE2007_ALG||\
	                               this->algorithm==_OFFICE2010_ALG||this->algorithm==_ORACLE7_10_ALG)
		this->encodeType = 1;		
	TiXmlElement *cipher_Num = head->FirstChildElement("Cipher_Num");
	this->cipher_num = atoi(cipher_Num->GetText());
	TiXmlElement *cipher_Len = head->FirstChildElement("Cipher_Len");
	this->cipher_len = atoi(cipher_Len->GetText());
	this->salt_len = 0;

	TiXmlElement *target = task_element->FirstChildElement("Target");
	uint8 cipher_index=0;
	while(target) {
        TiXmlElement *targetChildNode = target->FirstChildElement("HashValue");
	    strncpy(this->cipher[cipher_index],targetChildNode->GetText(),this->cipher_len);
		if(this->salt_len!=0) {
		  targetChildNode=target->FirstChildElement("Salt");
		  strncpy(this->salt[cipher_index],targetChildNode->GetText(),this->salt_len);
		}
		target=target->NextSiblingElement("Target");
		cipher_index++;
	}

	TiXmlElement *mode = task_element->FirstChildElement("Mode");
	this->indexfile_num=0;
	while(mode) {
		TiXmlElement *re = mode->FirstChildElement("RE");
		while(re) {
			this->indexfile_num++;
	  		strncpy(this->re[this->indexfile_num-1], re->GetText(),256);
	  		re = re->NextSiblingElement("RE");
		}
		mode = mode->NextSiblingElement("Mode");

	}

    return true;
}

//在此处创建并初始化RST文件
string ALGOriginalTask::createResultFile(TiXmlElement* const task_element, Task* task) {
	if (NULL == task_element) {
        LOG(ERROR) << "task_element is NULL";
        return NULL;
    } else if (NULL == task) {
    	LOG(ERROR) << "task is NULL";
        return NULL;
    }
    PlainTask *plain_task = static_cast<PlainTask*>(task);

    string rst_name = "";
    TiXmlDocument rst_doc;
    TiXmlElement *gpr_head = task_element->FirstChildElement("Head");
    
    TiXmlElement *times = gpr_head->FirstChildElement("Issue_Time");
    string time_str = times->GetText();


    TiXmlElement *rst_root = new TiXmlElement("RST");
	rst_doc.LinkEndChild(rst_root);
	TiXmlElement *rst_head = new TiXmlElement("Head");
	rst_root->LinkEndChild(rst_head);

	TiXmlElement *element = gpr_head->FirstChildElement("Version");
	TiXmlElement *rst_version = new TiXmlElement("Version");
	TiXmlText *rst_version_text = new TiXmlText(element->GetText());
	rst_version->LinkEndChild(rst_version_text);
	rst_head->LinkEndChild(rst_version);

	TiXmlElement *rst_type = new TiXmlElement("Type");
	TiXmlText *rst_type_text = new TiXmlText("RST");
	rst_type->LinkEndChild(rst_type_text);
	rst_head->LinkEndChild(rst_type);

	//获取系统当前时间，待更改
	TiXmlElement *rst_time = new TiXmlElement("User_Issue_Time");
	TiXmlText *rst_time_text = new TiXmlText(time_str.c_str());
	rst_time->LinkEndChild(rst_time_text);
	rst_head->LinkEndChild(rst_time);

	element = gpr_head->FirstChildElement("User_name");
	TiXmlElement *rst_user = new TiXmlElement("User_name");
	TiXmlText *rst_user_text = new TiXmlText(element->GetText());
	rst_user->LinkEndChild(rst_user_text);
	rst_head->LinkEndChild(rst_user);

	element = gpr_head->FirstChildElement("Sys_Task_ID");
	TiXmlElement *rst_task = new TiXmlElement("Sys_Task_ID");
	TiXmlText *rst_task_text = new TiXmlText(element->GetText());
	rst_task->LinkEndChild(rst_task_text);
	rst_head->LinkEndChild(rst_task);

	element = gpr_head->FirstChildElement("Algorithm");
	TiXmlElement *rst_alg = new TiXmlElement("Algorithm");
	TiXmlText *rst_alg_text = new TiXmlText(element->GetText());
	rst_alg->LinkEndChild(rst_alg_text);
	rst_head->LinkEndChild(rst_alg);
	plain_task->algorithm = atoi(element->GetText());

	element = gpr_head->FirstChildElement("Cipher_Num");
	TiXmlElement *rst_ciperNum = new TiXmlElement("Cipher_Num");
	TiXmlText *rst_ciperNum_text = new TiXmlText(element->GetText());
	rst_ciperNum->LinkEndChild(rst_ciperNum_text);
	rst_head->LinkEndChild(rst_ciperNum);
	plain_task->cipher_count = atoi(element->GetText());

	element = gpr_head->FirstChildElement("Cipher_Len");
	TiXmlElement *rst_ciperLen = new TiXmlElement("Cipher_Len");
	TiXmlText *rst_ciperLen_text = new TiXmlText(element->GetText());
	rst_ciperLen->LinkEndChild(rst_ciperLen_text);
	rst_head->LinkEndChild(rst_ciperLen);

	element = gpr_head->FirstChildElement("Comment");
	TiXmlElement *rst_comment = new TiXmlElement("Comment");
	string common_str = (element->GetText() == NULL) ? "" : element->GetText();
	TiXmlText *rst_comment_text = new TiXmlText(common_str.c_str());
	rst_comment->LinkEndChild(rst_comment_text);
	rst_head->LinkEndChild(rst_comment);

	TiXmlElement *rst_plainNum = new TiXmlElement("Plain_AllNum");
	TiXmlText *rst_plainNum_text = new TiXmlText("0");
	rst_plainNum->LinkEndChild(rst_plainNum_text);
	rst_head->LinkEndChild(rst_plainNum);

	TiXmlElement *rst_taskSpace = new TiXmlElement("Task_Space");
	TiXmlText *rst_taskSpace_text = new TiXmlText("0");
	rst_taskSpace->LinkEndChild(rst_taskSpace_text);
	rst_head->LinkEndChild(rst_taskSpace);

	TiXmlElement *rst_gpr_file = new TiXmlElement("GPR_FilePath");
	TiXmlText *rst_gpr_file_text = new TiXmlText(plain_task->task_file_name.c_str());
	rst_gpr_file->LinkEndChild(rst_gpr_file_text);
	rst_head->LinkEndChild(rst_gpr_file);

	element = gpr_head->FirstChildElement("UR_FilePath");
	TiXmlElement *rst_file = new TiXmlElement("UR_FilePath");
	TiXmlText *rst_file_text = new TiXmlText(element->GetText());
	rst_file->LinkEndChild(rst_file_text);
	rst_head->LinkEndChild(rst_file);

	struct timeval now_time;
	gettimeofday(&now_time, NULL);
	time_t tt = now_time.tv_sec;
	tm *temp = localtime(&tt);
	char now_time_str[32];
	sprintf(now_time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, 
			temp->tm_hour, temp->tm_min, temp->tm_sec);

	TiXmlElement *rst_beginTime = new TiXmlElement("T_Begin_Time");
	TiXmlText *rst_begin_text = new TiXmlText(now_time_str);
	rst_beginTime->LinkEndChild(rst_begin_text);
	rst_head->LinkEndChild(rst_beginTime);

	TiXmlElement *rst_finishTime = new TiXmlElement("T_Finish_Time");
	TiXmlText *rst_finish_text = new TiXmlText(now_time_str);
	rst_finishTime->LinkEndChild(rst_finish_text);
	rst_head->LinkEndChild(rst_finishTime);

	TiXmlElement *rst_foundTime = new TiXmlElement("LastFoundtime");
	TiXmlText *rst_foundTime_text = new TiXmlText(time_str.c_str());
	rst_foundTime->LinkEndChild(rst_foundTime_text);
	rst_head->LinkEndChild(rst_foundTime);

	TiXmlElement *rst_machine = new TiXmlElement("MachineTotal");
	TiXmlText *rst_machine_text = new TiXmlText("0");
	rst_machine->LinkEndChild(rst_machine_text);
	rst_head->LinkEndChild(rst_machine);

	TiXmlElement *gpr_target = task_element->FirstChildElement("Target");
	while(gpr_target) {
		TiXmlElement *rst_target = new TiXmlElement("Target");
		int32 sys_target_id = 0;
		gpr_target->Attribute("Sys_Target_ID",&sys_target_id);
		rst_target->SetAttribute("Sys_Target_ID",sys_target_id);

		int32 user_target_id = 0;
		gpr_target->Attribute("User_Target_ID",&user_target_id);
		rst_target->SetAttribute("User_Target_ID",user_target_id);

		int32 ur_target_id = 0;
		gpr_target->Attribute("UR_Target_ID",&ur_target_id);
		rst_target->SetAttribute("UR_Target_ID",ur_target_id);

		string user_task_id = "";
		user_task_id = gpr_target->Attribute("User_Task_ID");
		rst_name = user_task_id;
		rst_target->SetAttribute("User_Task_ID",user_task_id.c_str());

		element = gpr_target->FirstChildElement("HashValue");
		TiXmlElement *rst_hash = new TiXmlElement("HashValue");
        TiXmlText *rst_hash_text = new TiXmlText(element->GetText());
        rst_hash->LinkEndChild(rst_hash_text);
		rst_target->LinkEndChild(rst_hash);
	    
		if(this->salt_len != 0) {
		  	element = gpr_target->FirstChildElement("Salt");
			TiXmlElement *rst_salt = new TiXmlElement("Salt");
        	TiXmlText *rst_salt_text = new TiXmlText(element->GetText());
        	rst_salt->LinkEndChild(rst_salt_text);
			rst_target->LinkEndChild(rst_salt);
		}

		TiXmlElement *rst_hashPath = new TiXmlElement("HashPath");
        TiXmlText *rst_hashPath_text = new TiXmlText("/");
        rst_hashPath->LinkEndChild(rst_hashPath_text);
		rst_target->LinkEndChild(rst_hashPath);

		TiXmlElement *rst_targetSpace = new TiXmlElement("Target_Space");
        TiXmlText *rst_targetSpace_text = new TiXmlText("0");
        rst_targetSpace->LinkEndChild(rst_targetSpace_text);
		rst_target->LinkEndChild(rst_targetSpace);

		rst_root->LinkEndChild(rst_target);
		gpr_target=gpr_target->NextSiblingElement("Target");
	}
	string result_file_path = dcr::g_getConfig()->getResult_file_path();
	result_file_path = result_file_path + rst_name;
	plain_task->result_file_name = rst_name;
	rst_doc.SaveFile(result_file_path.c_str());
	return rst_name;
}

bool ALGGranularity::readFromFileInit(TiXmlElement *const granulity_element) {
    CHECK_NOTNULL(granulity_element);
    TiXmlElement *granularity = NULL;
    switch(algorithm_ID) {
    	case 0:
    		granularity = granulity_element->FirstChildElement("MD5");
    		break;
    	case 1:
    		granularity = granulity_element->FirstChildElement("Ntlm");
    		break;
    	case 2:
    		granularity = granulity_element->FirstChildElement("mscache");
    		break;
    	case 3:
    		granularity = granulity_element->FirstChildElement("Mysql");
    		break;
    	case 4:
    		granularity = granulity_element->FirstChildElement("Domino");
    		break;
    	case 5:
    		granularity = granulity_element->FirstChildElement("Unix_md5");
    		break;
    	case 6:
    		granularity = granulity_element->FirstChildElement("Unix_des");
    		break;
    	case 7:
    		granularity = granulity_element->FirstChildElement("Sha1");
    		break;
    	case 8:
    		granularity = granulity_element->FirstChildElement("");
    		break;
    	case 9:
    		granularity = granulity_element->FirstChildElement("Office7");
    		break;
    	case 10:
    		granularity = granulity_element->FirstChildElement("Office10");
    		break;
    	case 11:
    		granularity = granulity_element->FirstChildElement("Pdf");
    		break;
    	case 12:
    		granularity = granulity_element->FirstChildElement("wep");
    		break;
    	case 13:
    		granularity = granulity_element->FirstChildElement("");
    		break;
    	case 14:
    		granularity = granulity_element->FirstChildElement("Mediawiki");
    		break;
    	case 15:
    		granularity = granulity_element->FirstChildElement("Hwp");
    		break;
    	case 16:
    		granularity = granulity_element->FirstChildElement("Discus");
    		break;
    	case 17:
    		granularity = granulity_element->FirstChildElement("WordPress");
    		break;
    	case 18:
    		granularity = granulity_element->FirstChildElement("phpBB");
    		break;
    	case 19:
    		granularity = granulity_element->FirstChildElement("Oracle7_10");
    		break;
    	case 20:
    		granularity = granulity_element->FirstChildElement("Oracle11");
    		break;
    	case 21:
    		granularity = granulity_element->FirstChildElement("ssha");
    		break;
    	case 22:
    		granularity = granulity_element->FirstChildElement("WPA");
    		break;
    	case 23:
    		granularity = granulity_element->FirstChildElement("lotus");
    		break;
    	case 24:
    		granularity = granulity_element->FirstChildElement("MD5_16");
    		break;
    	case 25:
    		granularity = granulity_element->FirstChildElement("sha256");
    		break;
    	default:
    		break;
    }
    this->granularity = atoi(granularity->GetText());
    LOG(INFO) << " algorithm is " << algorithm_ID;
    return true;
}

/*****************************************ALGDecomposer*******************************************/
bool ALGDecomposer::isFinishGetTask() {
    return ((this->indexfile_index+1)>this->indexfile_num);
}

uint64 ALGDecomposer::getPlainNum(char* plainfile_path,uint8 plain_len)
{
  FILE* fp=fopen(plainfile_path,"rb");
  CHECK_NOTNULL(fp);
  fseek(fp,0L,SEEK_END);
  uint64 fsize=ftell(fp);
  fclose(fp); 
  return fsize/plain_len;
}

void ALGDecomposer::readIndexFile(char* indexfile_path,char* plainfile_path,uint64& totalPlainNum,
									uint8& plain_len,uint32& totalBlockNum,uint32& blockSize,uint32& blockGran) {
	//this->m_mutex->Lock();
    ifstream fin(indexfile_path);//读取文件
	if (!fin.is_open())
	{
		cout << "Error opening file"<<indexfile_path;
		exit (-1); 
	}
	string indexInfo;
	getline(fin,indexInfo);
	sscanf(indexInfo.c_str(),"%s %llu %u %u %u %u",plainfile_path,&totalPlainNum,&plain_len,&totalBlockNum,&blockSize,&blockGran);
	fin.close();
	//this->m_mutex->unLock();
}

ComputationTask* ALGDecomposer::getTask(int32 original_task_ID, OriginalTask* original_task,Granularity* granularity, bool is_first_get) { 
    CHECK_NOTNULL(original_task);
    CHECK_NOTNULL(granularity);

    ALGOriginalTask* ALG_original_task =
        static_cast<ALGOriginalTask*>(original_task);
    ALGGranularity* ALG_granularity =
        static_cast<ALGGranularity*>(granularity);
 
    ALGComputationTask* compute_task = new ALGComputationTask();
    if (is_first_get) {
		this->indexfile_num=ALG_original_task->indexfile_num;
	  	this->indexfile_index=0;
	  	this->block_offset=0;
	  	
	  	int32 *re_count = new int32[320];
	  	for (int32 i=0; i<320; i++) {
	  		re_count[i] = 0;
	  	}
	  	//vector<int32> vec(32,0);
	  	//this->m_re_decompose_count.insert(make_pair(original_task_ID, vec));
	  	this->m_re_decompose_count.insert(make_pair(original_task_ID, re_count));
	  	
	}
	compute_task->startBlockNo=this->block_offset;//block_offset
	char* start=strchr(ALG_original_task->re[this->indexfile_index],'{');
	char* end=strchr(ALG_original_task->re[this->indexfile_index],'}');
	char pLen[3]={NULL};
	char indexfilePath[256]={NULL};
	strncpy(pLen,start+1,end-start-1);
	compute_task->plain_len=atoi(pLen);//plain_len
	
	start=strchr(ALG_original_task->re[this->indexfile_index],'[');
	end=strchr(ALG_original_task->re[this->indexfile_index],']');
	strncpy(indexfilePath,start+1,end-start-1);
    compute_task->plain_num=ALG_granularity->granularity;//plain_num
	
	uint64 totalPlainNum=0;
	uint32 totalBlockNum=0;
	uint32 blockSize=0;
	uint32 blockGran=0;
	this->readIndexFile(indexfilePath,compute_task->encryptedPlainPath,totalPlainNum,compute_task->plain_len,totalBlockNum,blockSize,blockGran);
	
	compute_task->blockSize=blockSize;//blockSize
	compute_task->blockNum=ALG_granularity->granularity/blockGran;//blockNum 

	//记录任务的某一个表达式被分解成多少个任务。
   	map<int32, int32*>::iterator it = this->m_re_decompose_count.find(original_task_ID);
   	if (it != this->m_re_decompose_count.end()) {
   		int32* temp_count = it->second;
   		temp_count[indexfile_index]++;
   	}
   	compute_task->regulation_index = indexfile_index;

	if(this->block_offset+compute_task->blockNum>=totalBlockNum)//block_offset:number of assigned blocks
	{
	  compute_task->blockNum=totalBlockNum-this->block_offset;
	  compute_task->plain_num=totalPlainNum-this->block_offset*blockGran;
	  this->indexfile_index++;

	  this->block_offset=0;
	}else
	{
	  this->block_offset+=compute_task->blockNum;
	}
	
	/*
     cout<<"********************************************get computation task***************************************************"<<endl;
	 cout<<"encryptedPlainPath:"<<compute_task->encryptedPlainPath<<endl;	
	 cout<<"totalPlainNum:"<<totalPlainNum<<endl;
	 cout<<"totalBlockNum:"<<totalBlockNum<<endl;
	 cout<<"blockSize:"<<blockSize<<endl;
	 cout<<"blockGran:"<<blockGran<<endl;
	 cout<<"compute_task->plainNum:"<<(int)compute_task->plain_num<<endl;
	 cout<<"compute_task->plain_len:"<<(int)compute_task->plain_len<<endl;
	 cout<<"compute_task->startBlockNo:"<<(int)compute_task->startBlockNo<<endl;
     cout<<"compute_task->blockNum:"<<(int)compute_task->blockNum<<endl;
     */
    return compute_task;
}


void ALGDecomposer::updateResultFileByDecomposer(OriginalTask* original_task, int32 original_task_ID, 
												Granularity* granularity,  string result_file_path, bool is_first_get) {

	CHECK_NOTNULL(original_task);
    CHECK_NOTNULL(granularity);
    
	ALGOriginalTask* ALG_original_task = static_cast<ALGOriginalTask*>(original_task);
    ALGGranularity* ALG_granularity = static_cast<ALGGranularity*>(granularity);
    
    //初始化变量
    
    if (is_first_get) {
    	this->indexfile_index = 0;
    	for (int32 i = 0; i < 32; i ++) {
	  		this->isUpdate[i] = false;
	  	}
    }
    
    
    int32 re_index = this->indexfile_index;
    cout << "re index is " << re_index << endl;
    if(this->isUpdate[re_index]) {
    	return;
    }
    this->isUpdate[re_index] = true; 

    string regulation = string(ALG_original_task->re[re_index]);

    char *start = strchr(ALG_original_task->re[re_index], '[');
	char *end = strchr(ALG_original_task->re[re_index], ']');
	char index_file_path[256] = "NULL";
	strncpy(index_file_path, start + 1, end-start-1);
	cout << "index path is " << index_file_path << endl;
	
	//this->m_mutex->Lock();
	ifstream in_index;
	in_index.open(index_file_path);
	if(!in_index) {
		LOG(ERROR) << "open index file error";
		return;
	}
	string en_path = "";
	int64 plain_num_all = 0;
	in_index >> en_path >> plain_num_all;
	in_index.close();
	//this->m_mutex->unLock();
	
	
    //TiXmlDocument* result_doc = new TiXmlDocument(result_file_path.c_str());
    result_file_path = dcr::g_getConfig()->getResult_file_path() + result_file_path;
    cout << "result_file_path" << result_file_path << endl;
    TiXmlDocument result_doc(result_file_path.c_str());
    if (!(result_doc.LoadFile())) {
    	LOG(ERROR) << " Load rst file failed ";
        return;
    }
	TiXmlElement *root_element = result_doc.RootElement();
	TiXmlElement *target_element = root_element->FirstChildElement("Target");
	
	while (target_element) {
		
		TiXmlElement *search = new TiXmlElement("Searched_RE");
		
		TiXmlElement *search_re = new TiXmlElement("RE");
		TiXmlText *search_re_text = new TiXmlText(regulation.c_str());
		search_re->LinkEndChild(search_re_text);
		search->LinkEndChild(search_re);

		TiXmlElement *search_machine = new TiXmlElement("Machine");
		TiXmlText *search_machine_text = new TiXmlText("MIC2");
		search_machine->LinkEndChild(search_machine_text);
		search->LinkEndChild(search_machine);

		TiXmlElement *search_space = new TiXmlElement("RE_Space");
		TiXmlText *search_space_text = new TiXmlText(itoa(plain_num_all).c_str());
		search_space->LinkEndChild(search_space_text);
		search->LinkEndChild(search_space);

		TiXmlElement *search_time = new TiXmlElement("Begin_Time");
		struct timeval now_time;
		gettimeofday(&now_time, NULL);
		time_t tt = now_time.tv_sec;
		tm *temp = localtime(&tt);
		char time_str[32];
		sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, 
				temp->tm_hour, temp->tm_min, temp->tm_sec);
		TiXmlText *search_time_text = new TiXmlText(time_str);
		search_time->LinkEndChild(search_time_text);
		search->LinkEndChild(search_time);
		
		//更新target_space值
		TiXmlElement *space_element = target_element->FirstChildElement("Target_Space");
		int64 target_space = atoi(space_element->GetText());
		space_element->Clear();
		target_space += plain_num_all;
		TiXmlText *space_text = new TiXmlText(itoa(target_space).c_str());
		space_element->LinkEndChild(space_text);
	
		//更新task_space值
		TiXmlElement *head_element = root_element->FirstChildElement("Head");
		TiXmlElement *taskspace_element = head_element->FirstChildElement("Task_Space");
		taskspace_element->Clear();
		//space_element->LinkEndChild(space_text);
		TiXmlText *taskspace_text = new TiXmlText(itoa(target_space).c_str());
		taskspace_element->LinkEndChild(taskspace_text);
		
		
		target_element->LinkEndChild(search);
		
		target_element = target_element->NextSiblingElement("Target");
	}
	
	result_doc.SaveFile();
	
}

Result* ALGJob::computation(ComputationTask *task, OriginalTask *original_task) {
    CHECK_NOTNULL(task);
    CHECK_NOTNULL(original_task);
    ALGComputationTask *ALG_task = static_cast<ALGComputationTask*>(task);
    ALGOriginalTask *ALG_original_task = static_cast<ALGOriginalTask*>(original_task);
	ALGResult* ALG_Result=new ALGResult;//用于接受子进程返回的计算结果
	ALG_Result->init();
    cout<<endl<<"-----------------------------------------------------------start computation---------------------------------------------"<<endl;
    sighandler_t old_handler;
	old_handler=signal(SIGCHLD,SIG_DFL);
	string cacheStr = getCacheContext(task, original_task);
	//Job::m_lock.Lock();
	//string cacheStr = getContextToBeCached(task, original_task);
	//Job::m_lock.unLock();
	uint64 cache_len = cacheStr.length()+1;
	cout << "compute node cache len " << cache_len << endl;
    pid_t pid;//子进程ID
    int shmid;//共享内存ID
    ALGOriginalTask* ALG_shm_addr=NULL;
	ALGResult* ALG_Result_shm_addr=NULL;
	char *share_cache = NULL;
	void* shm_addr=NULL;//共享内存首地址
    if((shmid=shmget(IPC_PRIVATE,sizeof(ALGOriginalTask)+sizeof(ALGResult)+sizeof(char)*cache_len,IPC_CREAT|0640))<0)//创建当前进程的父子进程私有共享内存
	{
      perror("shmget");
      exit(-1);
    } 
	if((shm_addr=shmat(shmid,0,0))==(void*)-1) {
           perror("Parent:shmat");
           exit(-1);
    }
	else
		memset(shm_addr,NULL,sizeof(ALGOriginalTask)+sizeof(ALGResult)+sizeof(char)*cache_len); 
    switch(pid=fork())
	{
	  case -1:
	    perror("fork");
		exit(-1);
	  case 0://child
	  {
	    //构造ALG.out参数
		//argv[1]:shmId,argv[2]:encryptedPlainPath,argv[3]:startBlockNo,argv[4]:blockNum,argv[5]:palinNum,argv[6]:plainLen,argv[7]:secretKey,argv[8]:blockSize,argv[9]:devNo
		string shmId=itoa(shmid);
		string encryptedPlainPath=ALG_task->encryptedPlainPath;
		string startBlockNo=itoa(ALG_task->startBlockNo);
		string blockNum=itoa(ALG_task->blockNum);
		string plain_num=itoa(ALG_task->plain_num);
		string plain_len=itoa(ALG_task->plain_len);
		string secretKey="";//ALG_task->secretKey;
		string blockSize=itoa(ALG_task->blockSize);
		string cache_len_str = itoa(cache_len);


		const char* argv[11]={"./ALG.out",shmId.c_str(),encryptedPlainPath.c_str(),startBlockNo.c_str(),blockNum.c_str(),
								plain_num.c_str(),plain_len.c_str(),secretKey.c_str(),blockSize.c_str(),"0",cache_len_str.c_str()};
		cout<<"-----------------start the atomic charset computation-------------------"<<endl;
		execlp(argv[0], argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], argv[10], NULL);
		perror("execlp error!!!");
	    cout<<"-----------------end the atomic charset computation-------------------"<<endl;
	    exit(0);
	  }
	  default://parent
	  {
		//set share memory form 
		ALG_shm_addr=static_cast<ALGOriginalTask*>(shm_addr);
		pthread_mutex_init(&ALG_shm_addr->mutex,NULL); 
		perror("init in parent");
		pthread_mutex_lock(&ALG_shm_addr->mutex);
		perror("lock in parent ");
		//initialize originalTask
		ALG_shm_addr->algorithm=ALG_original_task->algorithm;
		ALG_shm_addr->encodeType=ALG_original_task->encodeType;
		ALG_shm_addr->cipher_num=ALG_original_task->cipher_num;
		ALG_shm_addr->cipher_len=ALG_original_task->cipher_len;
		ALG_shm_addr->salt_len=ALG_original_task->salt_len;
		//memcpy(ALG_shm_addr->task_name,ALG_original_task->task_name,20);
        memcpy(ALG_shm_addr->cipher,ALG_original_task->cipher,128*256);
		memcpy(ALG_shm_addr->salt,ALG_original_task->salt,128*32);
	    memcpy(ALG_shm_addr->re,ALG_original_task->re,32*256);

	    void* cache_p = shm_addr + sizeof(ALGOriginalTask);
	    share_cache = (char*)cache_p;
	    memcpy(share_cache, cacheStr.c_str(), cache_len);
		
		perror("in parent end write shm");
		pthread_mutex_unlock(&ALG_shm_addr->mutex);
		perror("in parent unlock");
		
		int status;
		//sighandler_t old_handler;
		old_handler=signal(SIGCHLD,SIG_DFL);

		cout << "child process wait " << endl;
		//下面有可能会出错
		int ret = waitpid(pid,&status,0);//waiting for the child process finish
		cout << " wait pid return is " << ret << endl;
		if (WCOREDUMP(status)) {
			cout << "core dump" << endl;
		}
		perror("in parent waitpid:");
		signal(SIGCHLD,old_handler);

		cout<<"child process exit normal with status: "<<status<<endl;
		ALG_Result_shm_addr=static_cast<ALGResult*>(shm_addr);
		//read the result form share memory
		memcpy(&ALG_Result->ts,&ALG_Result_shm_addr->ts,sizeof(struct timeval));
		memcpy(&ALG_Result->te,&ALG_Result_shm_addr->te,sizeof(struct timeval));

		ALG_Result->success_num=ALG_Result_shm_addr->success_num;
		memcpy(ALG_Result->ret_plain,ALG_Result_shm_addr->ret_plain,128*64);
		
		
		//删除父进程的共享内存映射地址
        if (shmdt(shm_addr)<0) {
            perror("Parent:shmdt");
            exit(1);
        }//else
            //printf("Parent: Deattach shared-memory.\n");
        if (shmctl(shmid,IPC_RMID,NULL)==-1)//delete the share memory
		{
			perror("shmct:IPC_RMID");
			exit(-1);
        }//else
          //printf("Delete shared-memory.\n");
	  }
	}//end fork
	
	ALG_Result->algorithm=ALG_original_task->algorithm;
	memcpy(ALG_Result->cipher,ALG_original_task->cipher,128*256);
    memcpy(ALG_Result->salt,ALG_original_task->salt,128*32);
    ALG_Result->regulation_index = ALG_task->regulation_index;
    cout << "-----------------------------------------------end computation----------------------------------------------------- " << endl;
    return ALG_Result;
}

void ALGJob::reduce(Result *compute_result, Result *reduce_result){
    
    CHECK_NOTNULL(compute_result);
    CHECK_NOTNULL(reduce_result);
    ALGResult *i_compute_result = static_cast<ALGResult*>(compute_result);
    ALGResult *i_reduce_result = static_cast<ALGResult*>(reduce_result);
	i_reduce_result->algorithm = i_compute_result->algorithm;
   	for(int i=0;i<MAX_CIPHER_NUM;i++)
    {
      if(i_compute_result->ret_plain[i][0] != NULL && i_compute_result->success_num > 0)
      { 
        memcpy(i_reduce_result->ret_plain[i], i_compute_result->ret_plain[i], MAX_PLAIN_LEN);		
        memcpy(i_reduce_result->cipher[i], i_compute_result->cipher[i], 256);	
        memcpy(i_reduce_result->salt[i], i_compute_result->salt[i], 32);
      }
    }
	i_reduce_result->success_num = 0;
	for(int i=0;i<MAX_CIPHER_NUM;i++)
    {
	    if(i_reduce_result->ret_plain[i][0] != NULL)
		  i_reduce_result->success_num ++;
	}  
}

//此函数可以被updataResultFile()替代
void ALGJob::saveResult(const vector<Result*>& final_result) {

    cout<<"save_result start"<<endl;
    uint8 result_num = final_result.size();
	cout<<"result_num:"<<final_result.size()<<endl;
    vector<ALGResult*> i_final_result(result_num);
    for (int32 i = 0; i < result_num; i++) {
        i_final_result[i] = static_cast<ALGResult*>(final_result[i]);
    }
    
    struct utsname buf;
    if (0 != uname(&buf)) 
	{
        *buf.nodename = '\0';
    }
    string result_file_name(buf.nodename);
    result_file_name.append(".xml");
    
    TiXmlDocument result_doc(result_file_name.c_str());
    TiXmlElement *root_element = new TiXmlElement("RST");
    result_doc.LinkEndChild(root_element);
    
    for (uint8 i = 0; i < result_num; i++) 
	{
        TiXmlElement *Result = new TiXmlElement("Result");
        root_element->LinkEndChild(Result);
		TiXmlElement *Algorithm=new TiXmlElement("Algorithm");
		Result->LinkEndChild(Algorithm);
		string res_algorithm=itoa(i_final_result[i]->algorithm);
		TiXmlText *AlgorithmText = new TiXmlText(res_algorithm.c_str());
		Algorithm->LinkEndChild(AlgorithmText);
		TiXmlElement *SuccessNum=new TiXmlElement("SuccessNum");
		Result->LinkEndChild(SuccessNum);
		string res_success_num=itoa(i_final_result[i]->success_num);
		TiXmlText *SuccessNumText = new TiXmlText(res_success_num.c_str());
		SuccessNum->LinkEndChild(SuccessNumText);
		for(uint8 ii=0;ii<MAX_CIPHER_NUM;ii++)
		{
		  if(i_final_result[i]->ret_plain[ii][0]!=NULL)
		  {
		    cout<<"cipher:"<<i_final_result[i]->cipher[ii]<<endl;
		    cout<<"plain:"<<i_final_result[i]->ret_plain[ii]<<endl;
			
		    TiXmlElement *Target=new TiXmlElement("Target");
			Result->LinkEndChild(Target);
			TiXmlElement *HashValue=new TiXmlElement("HashValue");
			Target->LinkEndChild(HashValue);
			TiXmlText *HashValueText = new TiXmlText(i_final_result[i]->cipher[ii]);
			HashValue->LinkEndChild(HashValueText);
			TiXmlElement *Salt=new TiXmlElement("Salt");
			Target->LinkEndChild(Salt);
			TiXmlText *SaltText = new TiXmlText(i_final_result[i]->salt[ii]);
			Salt->LinkEndChild(SaltText);
			TiXmlElement *PlainText=new TiXmlElement("PlainText");
			Target->LinkEndChild(PlainText);
			TiXmlText *Plain_Text = new TiXmlText(i_final_result[i]->ret_plain[ii]);
			PlainText->LinkEndChild(Plain_Text);
		  }
		}	
    }
    result_doc.SaveFile();
	cout<<"save_result end"<<endl;
}

string ALGJob::getCacheID(ComputationTask *task, OriginalTask *original_task) {
	CHECK_NOTNULL(task);
    CHECK_NOTNULL(original_task);
    ALGComputationTask *ALG_task = static_cast<ALGComputationTask*>(task);
    ALGOriginalTask *ALG_original_task = static_cast<ALGOriginalTask*>(original_task);
    string path = ALG_task->encryptedPlainPath;
    string plain_offset = itoa(ALG_task->startBlockNo);
    string cacheID = path + plain_offset;
	return cacheID;
}

///
/// \brief 返回需要缓存的内容，该内容可以从计算任务和原始任务中获得
///		   明文碰撞破解中，将任务所用到的库文件分片进行解密后返回即可。
///		   在Job::computation()函数中调用string getCacheContext(ComputationTask *task, OriginalTask *original_task)
///		   函数来获取缓存内容。
string ALGJob::getContextToBeCached(ComputationTask *task, OriginalTask *original_task) {
	CHECK_NOTNULL(task);
    CHECK_NOTNULL(original_task);
    ALGComputationTask *ALG_task = static_cast<ALGComputationTask*>(task);
    ALGOriginalTask *ALG_original_task = static_cast<ALGOriginalTask*>(original_task);
    string password = "R@n!=1*2*...*n@b6";
    string aesIV = "ABCDEF0123456789";
    byte* mes = md5(password);
    string cbcEncryptedText, strDigest;
	CalculateDigest(strDigest, mes);
	delete[] mes;

	string encryptedPlainPath = ALG_task->encryptedPlainPath;
	int32 startBlockNo = ALG_task->startBlockNo;
	int32 blockNum = ALG_task->blockNum;
	int64 blockSize = ALG_task->blockSize;

	ifstream in_en;
	in_en.open(encryptedPlainPath.c_str());
	if(!in_en) {
		LOG(ERROR) << "open index error" << endl;
		return NULL;
	}
	in_en.seekg(0, ios::end);
	int64 fsize = in_en.tellg();
    int64 offsetlong = (int64)(startBlockNo)*blockSize;

    
	string dePlain, destr;
	char buf[blockSize+1];
	memset(buf, 0, blockSize + 1);
	in_en.seekg(offsetlong, ios::beg);
	for(int i=0; i<blockNum; i++) {
		if(i == (blockNum-1) && ((fsize-offsetlong) < (int64)blockSize*blockNum)) {
			in_en.read(buf, fsize % blockSize);
			buf[fsize % blockSize] = '\0';
		}
		else {
			in_en.read(buf, blockSize);
			buf[blockSize] = '\0';
		}
		destr = CBC_AESDecryptStr(strDigest, aesIV, buf);
		memset(buf, 0, blockSize + 1);
		dePlain += destr;
	}
	in_en.close();
	return dePlain;
}

//更新RST文件
void ALGJob::updateResultFile(OriginalTask* original_task, int32 original_task_ID, Decomposer* decomposer,
												 Result *compute_result, Result *reduce_result, string path) {
	CHECK_NOTNULL(original_task);
	CHECK_NOTNULL(decomposer);
    CHECK_NOTNULL(compute_result);
    CHECK_NOTNULL(reduce_result);
    ALGOriginalTask *alg_original_task = static_cast<ALGOriginalTask*>(original_task);
	ALGDecomposer* ALG_decomposer = static_cast<ALGDecomposer*>(decomposer);
	ALGResult *i_compute_result = static_cast<ALGResult*>(compute_result);
    ALGResult *i_reduce_result = static_cast<ALGResult*>(reduce_result);

    int32 re_index = i_compute_result->regulation_index;
	string regulation_str = "";
	map<int32, int32*>::iterator it = ALG_decomposer->m_re_decompose_count.find(original_task_ID);
   	if (it != ALG_decomposer->m_re_decompose_count.end()) {
   		int32* temp_count = it->second;
   		temp_count[re_index]--;
   		int32 count = temp_count[re_index];
   		if(count == 0) {
   			regulation_str = string(alg_original_task->re[re_index]);

   		}
   	}

    Job::m_lock.Lock();
   	path = dcr::g_getConfig()->getResult_file_path() + path;
   	TiXmlDocument result_doc(path.c_str());
    if (!(result_doc.LoadFile())) {
    	LOG(ERROR) << " Load rst file failed ";
    	Job::m_lock.unLock();
        return;
    }

	TiXmlElement *root_element = result_doc.RootElement();
	TiXmlElement *target_element = root_element->FirstChildElement("Target");

	CrackXMLLog *c_xmlLog = dynamic_cast<CrackXMLLog*>(dcr::g_getXMLLog().get());
	int64 space_value = 0;
	bool finish_flag = false;
	int32 count = 0;
	while (target_element) {
		TiXmlElement *hashvalue = target_element->FirstChildElement("HashValue");
		string hash_str = hashvalue->GetText();

		TiXmlElement *salts = target_element->FirstChildElement("Salt");
		string salt_str = "";
		if (salts != NULL && salts->GetText() != NULL) {
			salt_str = salts->GetText();
		}

		string temp_str = string(i_compute_result->cipher[count]);
		//解出一条密文
		if ( (i_compute_result->ret_plain[count][0] != NULL) && (temp_str.compare(hash_str) == 0) && (i_compute_result->success_num > 0))  {
			TiXmlElement *plain_num_ele = target_element->FirstChildElement("Plain_Num");
			int32 plain_num = 1;
			if (NULL == plain_num_ele) {
				plain_num_ele = new TiXmlElement("Plain_Num");
				TiXmlText *plain_num_text = new TiXmlText("1");
				plain_num_ele->LinkEndChild(plain_num_text);
				target_element->LinkEndChild(plain_num_ele); 
			} else {
				plain_num = atoi(plain_num_ele->GetText());
				plain_num ++;
				plain_num_ele->Clear();
				TiXmlText *plain_num_text = new TiXmlText(itoa(plain_num).c_str());
				plain_num_ele->LinkEndChild(plain_num_text);

			}
			TiXmlElement *plain_target_ele = new TiXmlElement("Plain_Target");
			plain_target_ele->SetAttribute("Plain_Target_ID", plain_num);
			target_element->LinkEndChild(plain_target_ele);

			TiXmlElement *plain_text_ele = new TiXmlElement("Plain_Text");
			TiXmlText *plain_text = new TiXmlText(i_compute_result->ret_plain[count]);
			plain_text_ele->LinkEndChild(plain_text);
			plain_target_ele->LinkEndChild(plain_text_ele);

			TiXmlElement *plain_foundtime_ele = new TiXmlElement("FoundTime");
			struct timeval now_time;
			gettimeofday(&now_time, NULL);
			time_t tt = now_time.tv_sec;
			tm *temp = localtime(&tt);
			char time_str[32];
			sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, 
							temp->tm_hour, temp->tm_min, temp->tm_sec);
			TiXmlText *plain_foundtime_text = new TiXmlText(time_str);
			plain_foundtime_ele->LinkEndChild(plain_foundtime_text);
			plain_target_ele->LinkEndChild(plain_foundtime_ele);

			//Plain_AllNum字段加1
			TiXmlElement *head_element = root_element->FirstChildElement("Head");
			TiXmlElement *plain_allNum = head_element->FirstChildElement("Plain_AllNum");
			int32 all_num = atoi(plain_allNum->GetText()) + 1;
			plain_allNum->Clear();
			TiXmlText *plain_allNum_text = new TiXmlText(itoa(all_num).c_str());
			plain_allNum->LinkEndChild(plain_allNum_text);

			//修改LastFoundtime字段
			TiXmlElement *last_time = head_element->FirstChildElement("LastFoundtime");
			last_time->Clear();
			TiXmlText *last_time_text = new TiXmlText(time_str);
			last_time->LinkEndChild(last_time_text);

			//更新LOG中Match字段
			int32 sys_target_id = 0;
			target_element->Attribute("Sys_Target_ID",&sys_target_id);
			string user_task_id = "";
			user_task_id = target_element->Attribute("User_Task_ID");
			int32 user_target_id = 0;
			target_element->Attribute("User_Target_ID",&user_target_id);

			TiXmlElement *alg_element = head_element->FirstChildElement("Algorithm");
			int32 algorithm = atoi(alg_element->GetText());
					
			c_xmlLog->add_Match(original_task_ID, sys_target_id, user_task_id,
								 user_target_id, algorithm, hash_str, salt_str, i_compute_result->ret_plain[count]);

			//发送异步事件
			Task *result_task = dcr::g_getTaskManager()->getTaskByID(original_task_ID);
            int32 finish_packet_num = result_task->finish_packet_num;
            int32 send_packet_num = result_task->send_packet_num;
            int32 redispatch_packet_num = result_task->redispatch_packet_num;
            //如果任务未完成，则发送匹配事件
            if (send_packet_num != finish_packet_num + redispatch_packet_num) {
            	char absolute_path[1024];
				getcwd(absolute_path, 1024);
				string result_file_path = dcr::g_getConfig()->getResult_file_path();

            	Event *asyn_event = new Event();
            	asyn_event->setTask_id(original_task_ID);
            	asyn_event->setComputation_type("C");
            	asyn_event->setRst_filepath(string(absolute_path) + "/" + result_file_path + result_task->result_file_name);
           	 	asyn_event->setGpr_name(result_task->task_file_name);
            	asyn_event->setStatus(TASK_MATCH);
                asyn_event->setIdel(SYS_BUSY);
                cout << asyn_event->toJson() << endl;
                dcr::g_getAsynchronous()->sendEvent(asyn_event);
            	delete asyn_event;

            }

		}
		count ++;

		//添加Finished_Time
		TiXmlElement *search_re = target_element->FirstChildElement("Searched_RE");
		while (search_re) {
			TiXmlElement *re = search_re->FirstChildElement("RE");
			string re_temp = re->GetText();
			if (re_temp.compare(regulation_str.c_str()) == 0) {
				TiXmlElement *search_time = new TiXmlElement("Finished_Time");
				struct timeval now_time;
				gettimeofday(&now_time, NULL);
				time_t tt = now_time.tv_sec;
				tm *temp = localtime(&tt);
				char time_str[32];
				sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, 
						temp->tm_hour, temp->tm_min, temp->tm_sec);
				TiXmlText *search_time_text = new TiXmlText(time_str);
				search_time->LinkEndChild(search_time_text);
				search_re->LinkEndChild(search_time);

				//获取RE_Space
				TiXmlElement *re_space = search_re->FirstChildElement("RE_Space");
				space_value = atol(re_space->GetText());
			
				finish_flag = true;
				break;
			}
			search_re = search_re->NextSiblingElement("Searched_RE");
		}

		target_element = target_element->NextSiblingElement("Target");
	}

	//添加LOG中的Finish_RE,其中finish_reason有待考虑
	if (finish_flag) {
		c_xmlLog->add_Regulation_Finish(regulation_str, space_value, 0);
	}
	 
	//判断任务是否完成，如果完成则发送结束事件,并更新T_Finish_Time
	Task *result_task = dcr::g_getTaskManager()->getTaskByID(original_task_ID);
    int32 finish_packet_num = result_task->finish_packet_num;
    int32 send_packet_num = result_task->send_packet_num;
    int32 redispatch_packet_num = result_task->redispatch_packet_num;
    int32 decompose_num = result_task->decompose_packet_num;
    if (send_packet_num == finish_packet_num + redispatch_packet_num && finish_packet_num == decompose_num) {
    	char absolute_path[1024];
		getcwd(absolute_path, 1024);
		string result_file_path = dcr::g_getConfig()->getResult_file_path();
    	Event *asyn_event = new Event();
    	asyn_event->setTask_id(original_task_ID);
    	asyn_event->setComputation_type("C");
    	asyn_event->setRst_filepath(string(absolute_path) + "/" + result_file_path + result_task->result_file_name);
    	asyn_event->setGpr_name(result_task->task_file_name);
        asyn_event->setStatus(TASK_FINISH);
        if (dcr::g_getTaskManager()->isAllTaskFinish()) {
        	asyn_event->setIdel(SYS_IDEL);

        } else {
        	asyn_event->setIdel(SYS_BUSY);

        }
        cout << asyn_event->toJson() << endl;
   	 	dcr::g_getAsynchronous()->sendEvent(asyn_event);
    	delete asyn_event;

    	TiXmlElement *head_element = root_element->FirstChildElement("Head");
		TiXmlElement *t_finish_time_ele = head_element->FirstChildElement("T_Finish_Time");
		t_finish_time_ele->Clear();
		struct timeval now_time;
		gettimeofday(&now_time, NULL);
		time_t tt = now_time.tv_sec;
		tm *temp = localtime(&tt);
		char time_str[32];
		sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, 
						temp->tm_hour, temp->tm_min, temp->tm_sec);
		TiXmlText *t_finish_time_text = new TiXmlText(time_str);
		t_finish_time_ele->LinkEndChild(t_finish_time_text);
    	
    } 

    result_doc.SaveFile(path.c_str());
	Job::m_lock.unLock();

}

void ALGJob::updateResultFileByDecomposer(OriginalTask* original_task, int32 original_task_ID, Granularity* granularity, 
											Decomposer* decomposer, string path, bool is_first_get) {

	CHECK_NOTNULL(original_task);
    CHECK_NOTNULL(granularity);
    CHECK_NOTNULL(decomposer);
    
	ALGOriginalTask* ALG_original_task = static_cast<ALGOriginalTask*>(original_task);
    ALGGranularity* ALG_granularity = static_cast<ALGGranularity*>(granularity);
    ALGDecomposer* ALG_decomposer = static_cast<ALGDecomposer*>(decomposer);
    
    //初始化变量
    if(is_first_get) {
    	ALG_decomposer->indexfile_index = 0;
    	memset(ALG_decomposer->isUpdate, NULL, 32);
	  	for(int32 i=0; i<32; i++) {
	  		ALG_decomposer->isUpdate[i] = false;
	  	}
    }

    int32 re_index = ALG_decomposer->indexfile_index;
    if(ALG_decomposer->isUpdate[re_index]) {
    	return;
    }

    ALG_decomposer->isUpdate[re_index] = true; 

    string regulation = string(ALG_original_task->re[re_index]);

    char *start = strchr(ALG_original_task->re[re_index], '[');
	char *end = strchr(ALG_original_task->re[re_index], ']');
	char index_file_path[256] = "NULL";
	strncpy(index_file_path, start + 1, end-start-1);

	
	ifstream in_index;
	in_index.open(index_file_path);
	if(!in_index) {
		LOG(ERROR) << "open index file error";
		return;
	}
	string en_path;
	int64 plain_num_all;
	in_index >> en_path >> plain_num_all;
	in_index.close();

	Job::m_lock.Lock();
	path = dcr::g_getConfig()->getResult_file_path() + path;

    TiXmlDocument result_doc(path.c_str());
    if (!(result_doc.LoadFile())) {
    	LOG(ERROR) << " Load rst file failed ";
    	Job::m_lock.unLock();
        return;
    }
    
	TiXmlElement *root_element = result_doc.RootElement();
	TiXmlElement *target_element = root_element->FirstChildElement("Target");
	
	while (target_element) {
		
		TiXmlElement *search = new TiXmlElement("Searched_RE");
		
		TiXmlElement *search_re = new TiXmlElement("RE");
		TiXmlText *search_re_text = new TiXmlText(regulation.c_str());
		search_re->LinkEndChild(search_re_text);
		search->LinkEndChild(search_re);

		TiXmlElement *search_machine = new TiXmlElement("Machine");
		TiXmlText *search_machine_text = new TiXmlText("MIC2");
		search_machine->LinkEndChild(search_machine_text);
		search->LinkEndChild(search_machine);

		TiXmlElement *search_space = new TiXmlElement("RE_Space");
		TiXmlText *search_space_text = new TiXmlText(itoa(plain_num_all).c_str());
		search_space->LinkEndChild(search_space_text);
		search->LinkEndChild(search_space);

		TiXmlElement *search_time = new TiXmlElement("Begin_Time");
		struct timeval now_time;
		gettimeofday(&now_time, NULL);
		time_t tt = now_time.tv_sec;
		tm *temp = localtime(&tt);
		char time_str[32];
		sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, 
				temp->tm_hour, temp->tm_min, temp->tm_sec);
		TiXmlText *search_time_text = new TiXmlText(time_str);
		search_time->LinkEndChild(search_time_text);
		search->LinkEndChild(search_time);
		
		//更新target_space值
		TiXmlElement *space_element = target_element->FirstChildElement("Target_Space");
		int64 target_space = atoi(space_element->GetText());
		space_element->Clear();
		target_space += plain_num_all;
		TiXmlText *space_text = new TiXmlText(itoa(target_space).c_str());

		//更新task_space值
		TiXmlElement *head_element = root_element->FirstChildElement("Head");
		TiXmlElement *taskspace_element = head_element->FirstChildElement("Task_Space");
		taskspace_element->Clear();
		space_element->LinkEndChild(space_text);
		TiXmlText *taskspace_text = new TiXmlText(itoa(target_space).c_str());
		taskspace_element->LinkEndChild(taskspace_text);
		
		
		target_element->LinkEndChild(search);
		
		target_element = target_element->NextSiblingElement("Target");
	}
	
	result_doc.SaveFile();
	Job::m_lock.unLock();

	//更新LOG中RUN_RE字段
	CrackXMLLog *c_xmlLog = dynamic_cast<CrackXMLLog*>(dcr::g_getXMLLog().get());
	c_xmlLog->add_Regulation_Run(regulation);
	
}

void ALGJob::updateResultFileByTime(float time_rate) {
	int32 compute_node_num = 0;
	int32 group_num = dcr::g_getComputeNodeTable().get()->getGroupNum();
	GroupInTable *group_in_table = NULL;
	for (int32 i = 0; i < group_num; i++) {
        group_in_table = dcr::g_getComputeNodeTable().get()->getGroup(i);
        compute_node_num += group_in_table->getActiveComputeNode()->getLen();          
    }
   	//注意两个锁之间的加锁顺序
    Job::m_lock.Lock();
    dcr::g_getTaskManager()->lock_TaskMap();
    map<int32, Task*>* task_map = dcr::g_getTaskManager()->getTaskMap();
    map<int32, Task*>::iterator it = task_map->begin();
	for (; it != task_map->end(); it++) {
		Task *task = it->second;
		if (task != NULL && RUN == task->state && task->result_file_name != "/") {
			string result_file_path = dcr::g_getConfig()->getResult_file_path() + task->result_file_name;
    		TiXmlDocument result_doc(result_file_path.c_str());
    		if (!(result_doc.LoadFile())) {
    			LOG(ERROR) << " Load rst file failed ";
    			dcr::g_getTaskManager()->unLock_TaskMap();
    			Job::m_lock.unLock();
        		return;
   			}
   			TiXmlElement *root_element = result_doc.RootElement();
			TiXmlElement *headElement = root_element->FirstChildElement("Head");
			TiXmlElement *machineTotalElement = headElement->FirstChildElement("MachineTotal");

			float machine_total = atof(machineTotalElement->GetText());
			machine_total += time_rate * compute_node_num;

			char machine_total_num[16];
			memset(machine_total_num, 0, 16);
			sprintf(machine_total_num, "%.3f", machine_total);

			machineTotalElement->Clear();
			TiXmlText *machine_total_text = new TiXmlText(machine_total_num);
			machineTotalElement->LinkEndChild(machine_total_text);

			result_doc.SaveFile();

		}
    }
    dcr::g_getTaskManager()->unLock_TaskMap();
    Job::m_lock.unLock();


}

//更新LOG文件
void ALGJob::updateLogFile(Result *compute_result, Result *reduce_result) {

}

pair<int32, int32> ALGDecomposer::getTypePrior(OriginalTask* original_task,Granularity* granularity,
													 ComputationTask* compute_task, bool is_first_get){
	CHECK_NOTNULL(original_task);
    CHECK_NOTNULL(granularity);
    pair<int32, int32> type_prior_pair = make_pair(-1, -1);
    string type_resource_file_name = "type_resource.xml";
	if(is_first_get) {
    	if (!(this->type_doc.LoadFile(type_resource_file_name.c_str()))) {
    		LOG(ERROR) << " Load node_type file failed ";
        	return type_prior_pair;
    	}
	}

	string file = static_cast<ALGComputationTask*>(compute_task)->encryptedPlainPath;
	string machine = "MIC";
	TiXmlElement *root_element = this->type_doc.RootElement();
    TiXmlElement *type = root_element->FirstChildElement("Type");
    bool isfind = false;
    int32 type_seq;
    int32 type_prior;

    while(type){
        TiXmlElement *resource_elem = type->FirstChildElement("Resource");
        CHECK_NOTNULL(resource_elem);

        TiXmlElement *machine_elem = resource_elem->FirstChildElement("Machine");
        CHECK_NOTNULL(machine_elem);
        string temp_machine = machine_elem->GetText();

        TiXmlElement *file_elem = resource_elem->FirstChildElement("File");
        CHECK_NOTNULL(file_elem);

        //一个type中可能有多个file
        while(file_elem) {
        	string temp_file = file_elem->GetText();
        	if(temp_file == file && temp_machine == machine) {
        		isfind = true;

      			TiXmlElement *seq_ele = type->FirstChildElement("TypeSeq");
      			CHECK_NOTNULL(seq_ele);
      			type_seq = atoi(seq_ele->GetText());

      			TiXmlElement *prior_ele = type->FirstChildElement("Prior");
      			CHECK_NOTNULL(prior_ele);
      			type_prior = atoi(prior_ele->GetText());
        		break;
        	}
        	file_elem = file_elem->NextSiblingElement("File");
        }
        type = type->NextSiblingElement("Type");
    }

    if(!isfind) {
    	LOG(ERROR) << " can not find type ";
    	return type_prior_pair;
    }
	type_prior_pair = make_pair(type_seq, type_prior); 

	return type_prior_pair;
}

string PlainTask::toString() {
	Task::m_lock.Lock();
	string task_str = "";
	task_str += " task id is : ";
	task_str += itoa(task_id);
	task_str += "\n task_name is : ";
	task_str += task_name;
	task_str += "\n task_file_name is : ";
	task_str += task_file_name;
	task_str += "\n result_file_name is : ";
	task_str += result_file_name;
	task_str += "\n task_creater is : ";
	task_str += task_creater;
	task_str += "\n algorithm is : ";
	task_str += itoa(algorithm);
	task_str += "\n cipher_count is : ";
	task_str += itoa(cipher_count);

	task_str += "\n send_packet_num is : ";
	task_str += itoa(send_packet_num);
	task_str += "\n finish_packet_num is : ";
	task_str += itoa(finish_packet_num);
	task_str += "\n redispatch_packet_num is : ";
	task_str += itoa(redispatch_packet_num);

	task_str += "\n";
	Task::m_lock.unLock();
	return task_str;
}

bool CrackXMLLog::add_runJob(const Task& task) {
	
    const PlainTask *plain_task = dynamic_cast<PlainTask*>(const_cast<Task*>(&task));
	TiXmlDocument run_gpr;
	TiXmlElement *RUN_GPR=new TiXmlElement("RUN_GPR");
	run_gpr.LinkEndChild(RUN_GPR);
	
	TiXmlElement *Time = new TiXmlElement("Time");
	struct timeval now_time;
	gettimeofday(&now_time, NULL);
	time_t tt = now_time.tv_sec;
	tm *temp = localtime(&tt);
	char time_str[32]={NULL};
	sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);
	TiXmlText *time_text = new TiXmlText(time_str);
	Time->LinkEndChild(time_text);
	RUN_GPR->LinkEndChild(Time);
	
	TiXmlElement *Sys_Task_ID = new TiXmlElement("Sys_Task_ID");
	TiXmlText *sys_tast_id = new TiXmlText(itoa(plain_task->task_id).c_str());
	Sys_Task_ID->LinkEndChild(sys_tast_id);
	RUN_GPR->LinkEndChild(Sys_Task_ID);
	
	TiXmlElement *Action = new TiXmlElement("Action");
	string tasl_state="";
	switch(plain_task->state)
	{
		case CREATE:
			tasl_state="CREATE";
			break;
		case RUN:
			tasl_state="RUN";
			break;
		case FINISH:
			tasl_state="FINISH";
	}
	TiXmlText *action= new TiXmlText(tasl_state.c_str());
	Action->LinkEndChild(action);
	RUN_GPR->LinkEndChild(Action);
	
	TiXmlElement *Algorithm = new TiXmlElement("Algorithm");
	TiXmlText *algorithm = new TiXmlText(itoa(plain_task->algorithm).c_str());
	Algorithm->LinkEndChild(algorithm);
	RUN_GPR->LinkEndChild(Algorithm);
	
	TiXmlElement *Cipher_count = new TiXmlElement("Run_Cipher_Num");
	TiXmlText *cipher_count = new TiXmlText(itoa(plain_task->cipher_count).c_str());
	Cipher_count->LinkEndChild(cipher_count);
	RUN_GPR->LinkEndChild(Cipher_count);
	
	this->log_mutex->Lock();
	int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    run_gpr.Accept(&log_printer);

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

bool CrackXMLLog::add_Regulation_Run(const string& regulation) {
	
	if(regulation.length()==0)
		LOG(ERROR) << "regulation is NULL";
	TiXmlDocument run_re;
	TiXmlElement *RUN_RE=new TiXmlElement("RUN_RE");
	run_re.LinkEndChild(RUN_RE);
	
	TiXmlElement *Time = new TiXmlElement("Time");
	struct timeval now_time;
	gettimeofday(&now_time, NULL);
	time_t tt = now_time.tv_sec;
	tm *temp = localtime(&tt);
	char time_str[32]={NULL};
	sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);
	TiXmlText *time_text = new TiXmlText(time_str);
	Time->LinkEndChild(time_text);
	RUN_RE->LinkEndChild(Time);
	
	TiXmlElement *RE = new TiXmlElement("RE");
	TiXmlText *re = new TiXmlText(regulation.c_str());
	RE->LinkEndChild(re);
	RUN_RE->LinkEndChild(RE);
	
	this->log_mutex->Lock();

	int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    run_re.Accept(&log_printer);

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

bool CrackXMLLog::add_Regulation_Finish(const string& regulation, int64 re_space, int32 finish_reason) {
	
	if(regulation.length()==0||re_space==0)
		LOG(ERROR) << "regulation or re_space is NULL";
    TiXmlDocument finish_RE;
	TiXmlElement *Finish_RE=new TiXmlElement("Finish_RE");
	finish_RE.LinkEndChild(Finish_RE);
	
	TiXmlElement *Time = new TiXmlElement("Time");
	struct timeval now_time;
	gettimeofday(&now_time, NULL);
	time_t tt = now_time.tv_sec;
	tm *temp = localtime(&tt);
	char time_str[32]={NULL};
	sprintf(time_str, "%04d-%02d-%02d %02d:%02d:%02d", temp->tm_year + 1900, temp->tm_mon + 1, temp->tm_mday, temp->tm_hour, temp->tm_min, temp->tm_sec);
	TiXmlText *time_text = new TiXmlText(time_str);
	Time->LinkEndChild(time_text);
	Finish_RE->LinkEndChild(Time);
	
	TiXmlElement *RE = new TiXmlElement("RE");
	TiXmlText *re = new TiXmlText(regulation.c_str());
	RE->LinkEndChild(re);
	Finish_RE->LinkEndChild(RE);
	
	TiXmlElement *RE_Space = new TiXmlElement("RE_Space");
	TiXmlText *re_Space = new TiXmlText(itoa(re_space).c_str());
	RE_Space->LinkEndChild(re_Space);
	Finish_RE->LinkEndChild(RE_Space);
	
	TiXmlElement *Result = new TiXmlElement("Result");
	string result="";
	switch(finish_reason)
	{
		case RUN_ALL:
			result="RUN_ALL";
			break;
		case Matched_Break:
			result="Matched_Break";
			break;
		case Admin_Break:
			result="Admin_Break";
	}
	TiXmlText *result_text= new TiXmlText(result.c_str());
	Result->LinkEndChild(result_text);
	Finish_RE->LinkEndChild(Result);
	
	this->log_mutex->Lock();
	int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    finish_RE.Accept(&log_printer);

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

bool CrackXMLLog::add_Match(const int32 sys_task_ID, const int32 sys_target_ID, const string& user_task_ID, const int32 user_target_ID,
                    const int32 algorithm, const string& hashValue, const string& salt, const string& plain_text) {
	
	if(hashValue.length()==0||plain_text.length()==0)
		LOG(ERROR) << "hashValue or plain_text is NULL";
    TiXmlDocument match;
	TiXmlElement *Match=new TiXmlElement("Match");
	match.LinkEndChild(Match);
	
	TiXmlElement *Sys_Task_ID = new TiXmlElement("Sys_Task_ID");
	TiXmlText *sys_task_ID_text = new TiXmlText(itoa(sys_task_ID).c_str());
	Sys_Task_ID->LinkEndChild(sys_task_ID_text);
	Match->LinkEndChild(Sys_Task_ID);
	
	TiXmlElement *Sys_Target_ID = new TiXmlElement("Sys_Target_ID");
	TiXmlText *sys_Target_ID= new TiXmlText(itoa(sys_target_ID).c_str());
	Sys_Target_ID->LinkEndChild(sys_Target_ID);
	Match->LinkEndChild(Sys_Target_ID);
	
	TiXmlElement *User_Task_ID = new TiXmlElement("User_Task_ID");
	TiXmlText *user_Task_ID= new TiXmlText(user_task_ID.c_str());
	User_Task_ID->LinkEndChild(user_Task_ID);
	Match->LinkEndChild(User_Task_ID);
	
	TiXmlElement *User_Target_ID = new TiXmlElement("User_Target_ID");
	TiXmlText *user_Target_ID= new TiXmlText(itoa(user_target_ID).c_str());
	User_Target_ID->LinkEndChild(user_Target_ID);
	Match->LinkEndChild(User_Target_ID);
	
	TiXmlElement *Algorithm = new TiXmlElement("Algorithm");
	TiXmlText *algorithm_text = new TiXmlText(itoa(algorithm).c_str());
	Algorithm->LinkEndChild(algorithm_text);
	Match->LinkEndChild(Algorithm);
	
	TiXmlElement *HashValue = new TiXmlElement("HashValue");
	TiXmlText *hashValue_text= new TiXmlText(hashValue.c_str());
	HashValue->LinkEndChild(hashValue_text);
	Match->LinkEndChild(HashValue);
	
	TiXmlElement *Salt = new TiXmlElement("Salt");
	TiXmlText *salt_text= new TiXmlText(salt.c_str());
	Salt->LinkEndChild(salt_text);
	Match->LinkEndChild(Salt);
	
	TiXmlElement *Plain_Text = new TiXmlElement("Plain_Text");
	TiXmlText *plain_t= new TiXmlText(plain_text.c_str());
	Plain_Text->LinkEndChild(plain_t);
	Match->LinkEndChild(Plain_Text);
	
	this->log_mutex->Lock();
	int fd = open(this->log_file_path.c_str(), O_RDWR);
    if(-1 == fd) {
        cout << "Not found LOG file: " << endl;
        this->log_mutex->unLock();   
        return false;
    }
    TiXmlPrinter log_printer;
    match.Accept(&log_printer);

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
