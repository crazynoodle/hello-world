###purpose
主要是实现一个收集日志的简易服务器，功能主要是，一收集逻辑处理层产生的日志信息；二，将收集好的日志信息最终分类落入到对应的log文件中；

---
我的工作主要是：设计出一个大体的简易的框架，基于此框架，添加后续功能；其二，设计出合适客户端用的日志收集与及推送接口；

---
我的主要实现方法是：
采用Epoll+threadpoll+task_queueD的方式

---
模型设计如下：
我的主要特色设计如：

---
