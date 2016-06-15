#include "task_manager.h"
#include "../util/flags.h"
#include "../dcr/system_init.h"
int32 TaskManager::create_task(char *task_buf, int32 buf_len) {

	cout << "create a new task" << endl;
	Task *task = dcr::g_createTask();
	string task_str(task_buf);
	string::size_type pos = task_str.find(":::task_name:::");
	if (pos == task_str.npos) {
		return -1;
	}
	char *task_xml = task_buf + pos + strlen(":::task_name:::");
	char task_name[512];
	memset(task_name, 0, 512);
	memcpy(task_name, task_buf, pos);
	cout << " task name is " << task_name << endl;
	task->task_file_name = task_name;
	task->task_creater = "";
	this->task_mutex->Lock();
	this->global_task_id ++;
	task->task_id = this->global_task_id;
	task->state = CREATE;
	this->task_map->insert(make_pair(task->task_id, task));
	this->task_mutex->unLock();
	string task_file_path = dcr::g_getConfig()->getTask_file_path();
	task_file_path =  task_file_path + task_name;
	cout << "task file is " << task_file_path << endl;
	ofstream out_result;
	out_result.open(task_file_path);
	if(!out_result) {
		LOG(ERROR) << " open result file fail";
		return -1;
	}
	out_result << task_xml;
	out_result.close();
	LOG(INFO) << " write recv task file";
	return task->task_id;
}

int32 TaskManager::delete_task(int32 task_id) {

}

string TaskManager::search_task(int32 task_id) {
	Task *task = getTaskByID(task_id);
	return NULL == task ? NULL : task->toString();

}

int32 TaskManager::get_decomposer_state() {

}

int32 TaskManager::get_computernode_state() {

}

int32 TaskManager::get_global_task_ID() {
	return this->global_task_id;
}

Task* TaskManager::getTaskByID(int32 taskid) {
	this->task_mutex->Lock();
	map<int32, Task*>::iterator it = task_map->find(taskid);
	if (it != task_map->end()) {
		this->task_mutex->unLock();
		return it->second;
	}
    this->task_mutex->unLock();
    return NULL;
}

Task* TaskManager::getNextTask() {

	this->task_mutex->Lock();
	map<int32, Task*>::iterator it = task_map->begin();
	for (; it != task_map->end(); it++) {
		Task *task = it->second;
		if (task != NULL && CREATE == task->state) {
			task->state = RUN;
			this->task_mutex->unLock();
			return task;
		}
    }
    this->task_mutex->unLock();
    return NULL;
}

bool TaskManager::isAllTaskFinish() {
	bool is_finish = true;
	this->task_mutex->Lock();
	map<int32, Task*>::iterator it = task_map->begin();
	for (; it != task_map->end(); it++) {
		Task *task = it->second;
		if (task != NULL && FINISH != task->state) {
			is_finish = false;
		}
    }
    this->task_mutex->unLock();
    return is_finish;
}

bool TaskManager::init() {
	this->task_mutex = new Mutex();
    this->task_mutex->init();
	this->task_map = new map<int32, Task*>;
	return true;
}

bool TaskManager::destroy() {

	delete this->task_mutex;
	map<int32, Task*>::iterator it = task_map->begin();
    for (; it != task_map->end(); it++) {
        delete it->second;
        task_map->erase(it);
    }
    delete task_map;
    return true;
}

map<int32, Task*>* TaskManager::getTaskMap() {
	return this->task_map;
}

bool TaskManager::lock_TaskMap() {
	this->task_mutex->Lock();
	return true;
}

bool TaskManager::unLock_TaskMap() {
	this->task_mutex->unLock();
	return true;

}