#include "event.h"

string Event::toJson() {

  Json::Value root;  
  Json::FastWriter fast_writer;
  root["Status"]= Json::Value(status);
  root["RSTfpath"]= Json::Value(rst_filepath);
  root["GPRname"]= Json::Value(gpr_name);
  root["Taskid"]= Json::Value(task_id);
  root["MachineType"]= Json::Value(computation_type);
  root["Idle"]= Json::Value(idel);
  return fast_writer.write(root);
}

void Event::parseJson(string jsonstr){
  Json::Reader reader;  
    Json::Value root;  
    if (reader.parse(jsonstr, root)) {  
      this->status = root["Status"].asInt(); 
      this->rst_filepath = root["RSTfpath"].asString();    
      this->gpr_name = root["GPRname"].asString(); 
      this->task_id = root["Taskid"].asInt();    
      this->computation_type = root["MachineType"].asString();
      this->idel = root["Idle"].asInt();   
  } else {
    cout << "parse error " << endl;
  }
}