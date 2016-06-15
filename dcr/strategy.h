/*!
 * \file strategy.h
 * \brief 实现任务分发策略的可配置，可扩展
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.11.16
 */

#ifndef __DCR__strategy__
#define __DCR__strategy__

#include "../util/common.h"
#include "../core_data_structure/compute_node_table.h"
#include "../core_data_structure/functional_queue.h"
#include "../util/config.h"
#include "../asynchronous/client/asynchronous.h"

#include <unordered_map>
#include <list>
#include <vector>
#include <unordered_set>

///
/// \breif 可配置策略的基类
///
class Strategy {
public:
    
    Strategy() {};
    
    virtual ~Strategy() { this->destroy(); };
    
    virtual bool destroy() {
        //delete asyn;
        //LOG(INFO) << " destroy Asynchronous!";
        return true;
    }
    
    virtual bool init() { 
        //asyn = new Asynchronous();
        //asyn->setupClient("127.0.0.1", 9200);
        //LOG(INFO) << " init asynchronous!";
        return true;
    }
    
    ///
    /// \breif 实现分解策略可配置
    ///
    virtual void decomposeStrategy(Decomposer* ptr);
    
    ///
    /// \brief 实现dispacher策略可配置
    ///
    virtual void dispachStrategy(int group_seq);
    
    /// \brief 加入计算节点策略可配置
    ///
    /// 将m_login_list里的计算节点全部移入m_active_node_list
    /// \param login_list 登陆队列
    /// \param 要加入的计算节点分组
    /// \return 增加的计算节点的数量
    /// \see addOneNode
    virtual int32 addComputeNode(FunctionalQueue *login_list, GroupInTable* group);
    
    /// \brief 删除计算节点策略可配置
    ///
    /// 1.检查删除节点的IP是不是在active_ip_list中
    /// 2.如果不在则直接返回0
    /// 3.如果在则将node中的两个FunctionalQueue中的任务重新放回dcq
    /// \param delete_index 要删除的节点的索引
    /// \param decomposer_queue
    /// \param 计算节点分组
    /// \return 删除的节点数，成功返回1， 失败返回0
    virtual int32 deleteComputeNode(int32 delete_index, FunctionalQueue *decomposer_queue, GroupInTable* group);
    
    /// \brief 将超时的任务重新发送
    ///
    /// 检查m_active_node_list中的每个节点是否有超时的任务，有的话重新放回decomposer_queue
    /// \param decompose_queue 超时任务返回的地方
    /// \param 计算节点分组
    /// \return 需要redispatch的任务的数量
    /// \see redispatchOneNode
    virtual int32 redispatchTask(FunctionalQueue *decompose_queue, GroupInTable* group);
    
    /// \breif 实现任务加载的可配置策略
    ///
    /// 1.computeDispatchNum计算需要加载的任务的数量
    /// 2.从decompose_queue中加载任务到node的m_task_wait_to_send中
    /// \param node 需要加载任务的节点
    /// \param decompose_queue 从哪里加载任务
    /// \param 要加载的计算节点分组
    /// \return 加载的任务数量
    virtual int32 loadTask(ComputeNodeInfo *node, FunctionalQueue *decompose_queue, int group_seq = 0);
    
    /// \breif 实现任务发送可配置
    ///
    /// 1.computeSendNum计算需要发送的任务的数量
    /// 2.给要发送的udp_task赋予相关信息
    /// 3.调用udpComputationTaskSend进行发送
    /// 4.将m_task_wait_to_send中的任务赋予发送时间然后放入m_task_already_send
    //  5.如果m_task_wait_to_send中的task已经发送完或者发送的数量已经够了，则退出循环
    /// \param node 需要发送任务的节点
    /// \param index 节点的索引
    /// \param 要发送的计算节点分组
    /// \return 发送的任务数量
    virtual int32 sendTask(ComputeNodeInfo *node, int32 index, GroupInTable* group);
    
    /// \brief 实现节点检查的可配置
    ///
    /// 取得当前时间和计算节点最后一次的联系时间， 看看是否大于系统规定的时间
    /// \param node 需要检查的计算节点
    /// \return true则失去联系，false则没问题
    virtual bool checkDisconnect(ComputeNodeInfo *node);
    
    /// \brief 处理计算结果可配置
    ///
    /// 将result_list中的所有内容进行遍历处理
    /// \param result_list 结果队列
    /// \param 要假如的计算节点分组
    /// \return 处理的结果数量
    virtual int32 processResult(FunctionalQueue *result_list, GroupInTable* group);

    /// \brief 重发任务可配置
    ///
    /// 1.循环检查计算节点中的m_task_already_send的task， 看看是否超时
    /// 2.如果有一个超时则后面的内容不用检查
    /// 3.如果不超时则将task放入decompose_queue
    /// \param node 要重发的任务所属的节点
    /// \param decompose_queue 将超时的任务放入decompose_queue
    /// \param now 重发的时间
    /// \param redisp_one_node 一个节点中需要重发的任务的数量
    /// \return 重发成功true， 失败false
    virtual bool redispatchOneNode(ComputeNodeInfo *node, FunctionalQueue *decompose_queue,
                           time_t now, int32 &redisp_one_node, GroupInTable* group);
    
    /// \brief 加入一个节点
    ///
    /// 1.检查login_packet中的IP是否在active_ip_list中
    /// 2.如果在则关闭连接
    /// 3.如果没有则发送ack, 创建computeNodeInfo进行初始化加入active_node
    /// 将IP插入active_ip_list中
    /// \param login_list 登陆队列
    /// \param 要加入那个计算节点分组
    /// \return 增加的计算节点的数量
    bool addOneNode(LoginPacket *login_packet, time_t now, GroupInTable* group);
    
    
    
    /// \brief 处理一个结果
    ///
    /// 1.如果结果是heart_beat类型则对结果包中的信息进行检查，并更新对应node中的信息
    /// 2.结果是result则在第一结果返回时更新task_lost时间,对结果进行检查， 信息正确则更新对应node中信息并对结果进行归约
    ///
    /// \param result 返回的结果包
    /// \return 处理成功则返回true, 否则返回false
    bool processOneResult(SchedUdpRecv *result, time_t now, GroupInTable* group);
    
    /// \brief 检查这个分组中是不是所有的节点都已经完成任务
    ///
    /// 遍历分组中的所有节点, 如果所有节点的wait_to_send队列长度和already_send长度都为0，
    /// 而且dcq长度也为0则说明任务已经完成
    bool checkFinish(GroupInTable* group);
    
    ///
    /// \brief 计算应该往一个节点发送的任务的数量
    ///
    int32 computeSendNum(int32 cq_current);
    
    ///
    /// \brief 计算m_task_wait_to_send应该从dcq加载多少任务
    ///
    int32 computeDispatchNum(int32 wait_queue_len);

//protected:
    //Asynchronous *asyn;
    
};


///
/// \breif 根据任务类型进行发送的策略
///
class StrategyBlast: public Strategy {
public:
    StrategyBlast() {};
    
    ~StrategyBlast() { this->destroy(); };
    
    bool init();
    
    bool destroy();
    
    void decomposeStrategy(Decomposer* ptr);
    
    void dispachStrategy(int group_seq);
    ///
    /// \breif blast 的任务加载策略, 实现LRU算法
    ///
    /// 1.对每个新加入的节点创建一个map, 用于记录发送过的任务的历史, key为节点的ip, 每个ip对应了一个列表用于记录发送过的任务类型
    /// 2.如果ip的历史列表为空，则随机从m_type_index任务类型索引中随机选取任务加载, 并将最近加载的任务的类型放在历史队列的头部
    /// 3.如果不为空，则按照历史队列的任务类型(从头开始遍历)从节点中加载加载任务
    int32 loadTask(ComputeNodeInfo *node, FunctionalQueue *decompose_queue, int32 group_seq);
    
private:
    Mutex* m_mutex;                                             ///< 互斥锁
    unordered_multimap<int32, int32> m_type_index;              ///< 任务类型索引
    map<unsigned long, unordered_set<int32>*>* m_type_history;  ///< 每个节点接受过的计算任务类型记录
};


class StrategyType: public Strategy {
public:
    StrategyType() {};

    ~StrategyType() {this->destroy();};

    bool init();

    bool destroy();

    void decomposeStrategy(Decomposer* ptr);

    void dispachStrategy(int group_seq);

    int32 loadTask(ComputeNodeInfo *node, FunctionalQueue *decompose_queue, int32 group_seq);

    bool redispatchOneNode(ComputeNodeInfo *node, FunctionalQueue *decompose_queue,
                           time_t now, int32 &redisp_one_node, GroupInTable* group);

    int32 deleteComputeNode(int32 delete_index, FunctionalQueue *decomposer_queue, GroupInTable* group);

private:
    bool initNodeTypeMap();

    Mutex* m_mutex;                                    ///< 互斥锁
    unordered_multimap<int32, int32> m_type_index;
    map<unsigned long, unordered_set<int32>*> m_ip_type;      
};
#endif
