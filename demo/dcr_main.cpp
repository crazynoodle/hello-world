/*!
 * \file dcr_main.h
 * \brief 框架启动入口
 *
 * 通过参数指定IP，判断程序运行的节点是调度节点还是计算节点
 * 判断完成后分别进行初始化(分配内存空间)，完成之后进行作业
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */


#ifndef __DCR__dcr_main__
#define __DCR__dcr_main__
#include "../gflags/gflags.h"
#include "../util/logger.h"
#include "../dcr/system_init.h"
 #include "../util/timer.h"

int main(int argc, char** argv){
    
    google::ParseCommandLineFlags(&argc, &argv, false);
    //如果是调度节点
    if (dcr::g_isSchedulerNode()) {
        if (dcr::g_SchedulerNodeInitialize()) {
            LOG(INFO) << "I am a Scheduler";
            Timer timer;
            timer.startTick();
            //dcr::g_getXMLLog()->init();
            dcr::g_getXMLLog()->createLOG();
            dcr::g_getXMLLog()->add_StartTime();

            dcr::g_SchedulerWork();
            dcr::g_SchedulerFinilize();
            
            dcr::g_getXMLLog()->add_ShutdownTime();
            timer.finishTick();
			double diff = dcr::g_getConfig()->getSchedulerTime() * dcr::g_getConfig()->getRedispatchTime();
            LOG(INFO) << " cost time: " << timer.getDuration() - diff;
        } else {
            LOG(ERROR) << "Init SchedulerNode Failed!";
            return -1;
        }
    } else {    //如果是计算节点
        if (dcr::g_ComputeNodeInitialize()) {
            LOG(INFO) << "I am a ComputeNode";
            dcr::g_ComputeWork();
            dcr::g_ComputeFinilize();
        } else {
            LOG(ERROR) << "Init ComputeNode Failed";
            return -1;
        }
    }
}

#endif 