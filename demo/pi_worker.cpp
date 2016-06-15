#include "../demo/pi_worker.h"

#include <cmath>

bool IntegrationDecomposer::isFinishGetTask() {
    return (this->m_current_value > this->m_max_value);
}

ComputationTask* IntegrationDecomposer::getTask(OriginalTask* original_task,
                                                Granulariry* granulity, bool is_first_get) {
    CHECK_NOTNULL(original_task);
    CHECK_NOTNULL(granulity);
    IntegrationOriginalTask* integration_original_task = static_cast<IntegrationOriginalTask*>(original_task);
    IntegrationGranularity* integration_granulity = static_cast<IntegrationGranularity*>(granulity);
    IntegrationComputationTask* compute_task = new IntegrationComputationTask();
    if (is_first_get) {
        this->m_max_value = integration_original_task->m_upper_bound;
        this->m_current_value = integration_original_task->m_lower_bound;
        compute_task->m_lower_bound = this->m_current_value;
        float next_value = this->m_current_value + integration_granulity->m_granulity;
        compute_task->m_upper_bound = next_value;
        this->m_current_value = next_value;
    } else {
        compute_task->m_lower_bound = this->m_current_value;
        float next_value = this->m_current_value + integration_granulity->m_granulity;
        compute_task->m_upper_bound = next_value;
        this->m_current_value = next_value;
    }
    return compute_task;
}


bool IntegrationGranularity::readFromFileInit(TiXmlElement *const granulity_element) {
    CHECK_NOTNULL(granulity_element);
    TiXmlElement *granulity_val = granulity_element->FirstChildElement("val");
    this->m_granulity = atof(granulity_val->GetText());
    return true;
}


bool IntegrationOriginalTask::readFromFileInit(TiXmlElement * const task_element) {
    if (task_element == NULL) {
        LOG(ERROR) << "task_element is NULL";
        return false;
    }
    TiXmlElement *upper_bound = task_element->FirstChildElement("upper_bound");
    this->m_upper_bound = atof(upper_bound->GetText());
    TiXmlElement *lower_bound = task_element->FirstChildElement("lower_bound");
    this->m_lower_bound = atof(lower_bound->GetText());
    TiXmlElement *step = task_element->FirstChildElement("step");
    this->m_step = atof(step->GetText());
    return true;
}

bool IntegrationResult::init() {
    this->m_result = 0.0;
    return true;
}


Result* IntegrationJob::computation(ComputationTask *task, OriginalTask *orginal_task) {
    
    IntegrationComputationTask *integration_task = static_cast<IntegrationComputationTask*>(task);
    IntegrationOriginalTask *integration_original_task = static_cast<IntegrationOriginalTask*>(orginal_task);
    
    float lower_bound = integration_task->m_lower_bound;
    float upper_bound = integration_task->m_upper_bound;
    
    float step = integration_original_task->m_step;
    int64 num = (upper_bound - lower_bound) / step;
    float result = 0.0;
    float iter = lower_bound;
    float test = 1.0;
    for (int64 i = 0; i < num; i++) {
        result += sin(iter);
        iter += step;
        for (int64 j = 0; j < 1000; j++) {
            test *= -1;
        }
        
    }
    result = result + test;
    IntegrationResult *ret = new IntegrationResult();
    ret->m_result = result;
    return ret;
}

void IntegrationJob::reduce(Result *compute_result, Result *reduce_result) {
    
    CHECK_NOTNULL(compute_result);
    CHECK_NOTNULL(reduce_result);
    
    IntegrationResult *i_compute_result = static_cast<IntegrationResult*>(compute_result);
    IntegrationResult *i_reduce_result = static_cast<IntegrationResult*>(reduce_result);
    
    i_reduce_result->m_result += i_compute_result->m_result;
}

void IntegrationJob::saveResult(Result* final_result, TiXmlElement* const element) {
    IntegrationResult *i_final_result = static_cast<IntegrationResult*>(final_result);
    float data = i_final_result->m_result;
    char *c_data = new char[10];
    sprintf(c_data, "%f", data);
    TiXmlElement* data_element = new TiXmlElement("Result");
    element->LinkEndChild(data_element);
    TiXmlText* data_text = new TiXmlText(c_data);
    data_element->LinkEndChild(data_text);
}