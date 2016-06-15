#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <memory>
#include <algorithm>

#include "blast.h"

using namespace std;

bool BlastResult::init()
{
    memset(m_qseqid, sizeof(m_qseqid), 0);
    memset(m_sseqid, sizeof(m_sseqid), 0);
    m_bitscore = -1;
    return true;
}


void BlastJob::reduce(Result *compute_result, Result *reduce_result)
{
    CHECK_NOTNULL(compute_result);
    CHECK_NOTNULL(reduce_result);

    BlastResult *blast_compute_result = 
        static_cast<BlastResult*>(compute_result);
    BlastResult *blast_reduce_result = 
        static_cast<BlastResult*>(reduce_result);

    if (blast_compute_result->m_bitscore > blast_reduce_result->m_bitscore) {
        memcpy(&(blast_reduce_result->m_qseqid), &(blast_compute_result->m_qseqid), 20);
        memcpy(&(blast_reduce_result->m_sseqid), &(blast_compute_result->m_sseqid), 20);
        blast_reduce_result->m_bitscore = blast_compute_result->m_bitscore;
    }
    
}



void BlastJob::saveResult(Result *final_result, TiXmlElement* const element)
{
    CHECK_NOTNULL(final_result);

    BlastResult *blast_final_result = static_cast<BlastResult*>(final_result);
    
    TiXmlElement* qseqid = new TiXmlElement("qseqid");
    element->LinkEndChild(qseqid);
    TiXmlText* qseqid_text = new TiXmlText(blast_final_result->m_qseqid);
    qseqid->LinkEndChild(qseqid_text);
    
    TiXmlElement* sseqid = new TiXmlElement("sseqid");
    element->LinkEndChild(sseqid);
    TiXmlText* sseqid_text = new TiXmlText(blast_final_result->m_sseqid);
    sseqid->LinkEndChild(sseqid_text);
    
    TiXmlElement* bitscore = new TiXmlElement("bitscore");
    element->LinkEndChild(bitscore);
    char *bitscore_data = new char[8];
    sprintf(bitscore_data, "%d", blast_final_result->m_bitscore);
    TiXmlText* bitscore_text = new TiXmlText(bitscore_data);
    bitscore->LinkEndChild(bitscore_text);

}

// 将数字转换为字符串，如果是小于10的数字，前面添加一个0
void BlastJob::convertToFileOrder(char* file_order)
{
    if (position < 0) {
        fprintf(stderr, "the position can not be negative!");
        return;
    }

    char buf[33];
    const char numbers[] = "9876543210123456789";
    char *ptr = buf;
    char *ptr1 = buf;
    char tmp_char;
    int tmp_value;

    if (position < 10) {
        *ptr++ = '0';
        ptr1++;
    }

    do {
        tmp_value = position;
        position /= 10;
            *ptr++ = numbers[9 + (tmp_value - position * 10)];
    } while (position);

    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    strcpy(file_order, buf);
}

const char* BlastJob::generateDBname(char *dbname)
{
    char file_order[20];
    convertToFileOrder(file_order);
    strcat(dbname, ".");
    strcat(dbname, file_order);
    return dbname;
}

Result* BlastJob::computation(ComputationTask *task,
        OriginalTask *original_task)
{
    CHECK_NOTNULL(task);
    CHECK_NOTNULL(original_task);

    BlastComputationTask *blast_task = 
        static_cast<BlastComputationTask*>(task);
    BlastOriginalTask *blast_original_task = 
        static_cast<BlastOriginalTask*>(original_task);

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
    }
    cerr << "start fork\n";
    switch (fork()) {
    case -1:
        perror("fork");
        exit(0);

    case 0: // child
        { // 如果不加块区域，编译器会报错说step没有初始化
        close(fd[0]);
        if (fd[1] != STDOUT_FILENO) {
            // 将标准输出重定向到管道上，倒到父进程里面
            if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
                perror("dup2 in child");
            }
            close(fd[1]);
        }
            
        cerr << "child\n";
        int step = blast_original_task->m_step;
        this->position = blast_task->m_start + step - 1;
        const char* argv[12] = {
            blast_original_task->m_command,
            "-query",
            blast_original_task->m_queryFilename,
            "-db",
            this->generateDBname(blast_original_task->m_dbname),
            "-task",
            blast_original_task->m_command,
            "-outfmt",
            "6 qseqid sseqid bitscore",
            "-max_target_seqs",
            "1"
        };
            
        execlp(argv[0], argv[0], argv[1], argv[2], argv[3],
               argv[4], argv[5], argv[6], argv[7], argv[8],
               argv[9], argv[10], NULL);
        perror("execlp");
        exit(1);
        }

    default: // parent
        close(fd[1]);

        const int bufsize = 4096;
        char buf[bufsize]; // 需要刚好能显示完一次查询的输出
        // 只取每项第一行的结果
        int num;
        BlastResult *r = new BlastResult();
        cerr << "parent start" << endl;
        // bufsize必须足够大以容纳至少每项的第一行的结果
        if((num = (int)read(fd[0], buf, bufsize)) > 0) {
            //cerr << " buf: " << buf << endl;
            int i;
            bool newLineFound = false;
            for (i = 0; i < num; ++i) {
                if (buf[i] == '\n') {
                    newLineFound = true;
                    break;
                }
            }
            if (newLineFound) {
                istringstream strbuf(string(buf, i));
                strbuf >> r->m_qseqid >> r->m_sseqid >> r->m_bitscore;
                cerr << "string " << string(buf, i) << endl;
            }
            // 否则丢掉这次输出
        }
        if (num < 0) {
            perror("read");
        }

        cerr << "parent end" << endl;
        wait(0);
        return r;
    }
}

bool BlastOriginalTask::readFromFileInit(TiXmlElement * const task_element)
{
    if (task_element == NULL) {
        LOG(ERROR) << "task_element is NULL";
        return false;
    }
    TiXmlElement *start = task_element->FirstChildElement("start");
    this->m_start = atoi(start->GetText());
    TiXmlElement *end = task_element->FirstChildElement("end");
    this->m_end = atoi(end->GetText());
    TiXmlElement *step = task_element->FirstChildElement("step");
    this->m_step = atoi(step->GetText());
    TiXmlElement *command = task_element->FirstChildElement("command");
    strcpy(this->m_command, command->GetText());
    TiXmlElement *queryFilename = task_element->FirstChildElement("queryFilename");
    strcpy(m_queryFilename, queryFilename->GetText());
    TiXmlElement *dbname = task_element->FirstChildElement("dbname");
    strcpy(m_dbname, dbname->GetText());
    TiXmlElement *type = task_element->FirstChildElement("type");
    this->m_type = atoi(type->GetText());

    return true;
}

bool BlastGranularity::readFromFileInit(TiXmlElement * const granulity_element)
{
    CHECK_NOTNULL(granulity_element);
    TiXmlElement *granulity_val = granulity_element->FirstChildElement("val");
    this->m_granularity = atoi(granulity_val->GetText());
    return true;
}

ComputationTask* BlastDecomposer::getTask(OriginalTask *original_task,
        Granulariry* granulity, bool is_first_get)
{
    CHECK_NOTNULL(original_task);
    CHECK_NOTNULL(granulity);
    
    BlastOriginalTask* blast_original_task = 
        static_cast<BlastOriginalTask*>(original_task);
    BlastGranularity* blast_granulity = 
        static_cast<BlastGranularity*>(granulity);
    BlastComputationTask *compute_task = new BlastComputationTask();

    if (is_first_get) {
        this->m_max_value = blast_original_task->m_end;
        this->m_current_value = blast_original_task->m_start;
    }

    compute_task->m_start = this->m_current_value;
    int next_value = this->m_current_value + blast_granulity->m_granularity;
    compute_task->m_end = next_value;
    this->m_current_value = next_value;

    cerr << "ComputationTask " << this->m_current_value << endl;
    return compute_task;
}

bool BlastDecomposer::isFinishGetTask() 
{
    cerr << "check finished? " << m_max_value << "\n";
    return m_current_value == m_max_value;
}
