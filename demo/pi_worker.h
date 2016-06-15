#ifndef DCR_pi_worker_h
#define DCR_pi_worker_h

#include "../dcr/dcr_base_class.h"


class IntegrationDecomposer : public Decomposer {
public:
    bool isFinishGetTask();
    
    ComputationTask *getTask(OriginalTask* original_task, Granulariry* granulity, bool is_first_get);
    
public:
    float m_current_value;
    float m_max_value;
};
REGISTER_DECOMPOSER(IntegrationDecomposer);


class IntegrationOriginalTask : public OriginalTask {
public:
    bool readFromFileInit(TiXmlElement *const task_element);
    
public:
    float m_upper_bound;
    float m_lower_bound;
    float m_step;
};
REGISTER_ORIGINAL_TASK(IntegrationOriginalTask);


class IntegrationGranularity : public Granulariry {
public:
    bool readFromFileInit(TiXmlElement * const task_element);
    
public:
    float m_granulity;
};
REGISTER_GRANULITY(IntegrationGranularity);


class IntegrationComputationTask : public ComputationTask {
public:
    float m_upper_bound;
    float m_lower_bound;
};
REGISTER_COMPUTATION_TASK(IntegrationComputationTask);


class IntegrationResult : public Result {
public:
    bool init();
    float m_result;
};
REGISTER_RESULT(IntegrationResult);


class IntegrationJob : public Job {
public:
    Result *computation(ComputationTask *task, OriginalTask *original_task);
    
    void reduce(Result *compute_result, Result *reduce_result);
    
    void saveResult(Result *final_result, TiXmlElement* const result_elemet);
};
REGISTER_JOB(IntegrationJob);


#endif
