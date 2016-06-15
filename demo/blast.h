#ifndef DCR_BLAST_H
#define DCR_BLAST_H

#include "../dcr/dcr_base_class.h"
#include <vector>
#include <map>
#include <string>

class BlastResult : public Result
{
public:
    bool init();
    
    char m_qseqid[20];
    char m_sseqid[20];
    int m_bitscore;
    
};
REGISTER_RESULT(BlastResult);


class BlastOriginalTask : public OriginalTask
{
public:
    bool readFromFileInit(TiXmlElement* const task_element);
    
    int32 getType() {
        return m_type;
    }
    
    int32 m_type;
    int m_start;
    int m_step;
    int m_end;
    char m_command[20];
    char m_queryFilename[20];
    char m_dbname[20];
};
REGISTER_ORIGINAL_TASK(BlastOriginalTask);

class BlastComputationTask : public ComputationTask
{
public:
    int m_start;
    int m_end;
};
REGISTER_COMPUTATION_TASK(BlastComputationTask);

class BlastGranularity : public Granulariry
{
public:
    bool readFromFileInit(TiXmlElement * const task_element);
    int m_granularity;
};
REGISTER_GRANULITY(BlastGranularity);

class BlastJob : public Job
{
public:
    Result* computation(ComputationTask *task, OriginalTask *orginal_task);

    void reduce(Result *compute_result, Result *reduce_result);
    void saveResult(Result *final_result, TiXmlElement* const result_elemet);

private:
    const char* generateDBname(char* dbname);
    void convertToFileOrder(char* file_order);
    int position;
};
REGISTER_JOB(BlastJob);

class BlastDecomposer : public Decomposer
{
public:
    bool isFinishGetTask();
    ComputationTask* getTask(OriginalTask *original_task, Granulariry* granulity, bool is_first_get);

private:
    int step;
    int m_current_value;
    int m_max_value;
};
REGISTER_DECOMPOSER(BlastDecomposer);

#endif /* BLAST_H */

