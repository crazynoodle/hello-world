#ifndef Asy_event__
#define Asy_event__

#include <string>
#include <cstring>
#include "../json/json.h"

using namespace std;
class Event {
public:
    
    Event() {};

    //Event(string jsonstr) {};
    
    virtual ~Event() { this->destroy(); };
    
    virtual bool destroy() { return true; };

    virtual string toJson();

    virtual void parseJson(string jsonstr);
    
    void setStatus(int status) {
        this->status = status;
    }

    void setTask_id(int task_id) {
        this->task_id = task_id;
    }

    void setIdel(int idel) {
        this->idel = idel;
    }
    
    void setRst_filepath(string rst_filepath) {
        this->rst_filepath = rst_filepath;
    }

    void setGpr_name(string gpr_name) {
        this->gpr_name = gpr_name;
    }

    void setComputation_type(string computation_type) {
        this->computation_type = computation_type;
    }

protected:
    int status;
    int task_id;
    int idel;
    string rst_filepath;
    string gpr_name;
    string computation_type;
};


#endif