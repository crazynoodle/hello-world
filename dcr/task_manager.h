#ifndef __DCR__task_manager__
#define __DCR__task_manager__


#include "../util/common.h"
#include "../util/config.h"
#include "../util/logger.h"
#include "../util/mutex.h"
#include "../util/scoped_ptr.h"
#include "dcr_base_class.h"


#include <map>
#include <string>

class Task;

class TaskManager {

public:
	TaskManager():decomposer_queue_len(0), global_task_id(0)
	{};

	~TaskManager() {
		this->destroy();
	};

	bool init();
	
	int32 create_task(char *task_buf, int32 buf_len);

	int32 delete_task(int32 task_id);

	string search_task(int32 task_id);

	int32 get_decomposer_state();

	int32 get_computernode_state();

	int32 get_global_task_ID();

	Task* getTaskByID(int32 task_id);

	Task* getNextTask();

	bool isAllTaskFinish();

	map<int32, Task*>* getTaskMap();

	bool lock_TaskMap();

	bool unLock_TaskMap();

private:
	bool destroy();

	Mutex* task_mutex;  
	map<int32, Task*> *task_map;
	int32 decomposer_queue_len;
	int32 global_task_id;

};



#endif