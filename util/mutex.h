/*!
 * \file mutex.h
 * \brief 对互斥锁进行封装
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */
#ifndef __DCR__mutex__
#define __DCR__mutex__

#include <errno.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>

#include "../util/common.h"
#include "../util/logger.h"

class ConditionVariable;

using namespace std;

///
/// \brief 互斥锁的资源管理类, 暂时没用到
///
template <typename LockType>
class ScopedLocker {
public:
    explicit ScopedLocker(LockType* lock) : m_lock(lock) {
        m_lock->Lock();
    }
    
    ~ScopedLocker() {
        m_lock->Unlock();
    }
    
private:
    LockType* m_lock;
};

///
/// \brief 互斥锁的封装
///
class Mutex {
public:
    explicit Mutex() {
        
    }
    
    ~Mutex() {
        pthread_mutex_destroy(&m_Mutex);
    }
    
    void init() {
        int32 error_t;
        error_t = pthread_mutex_init(&m_Mutex, NULL);
        checkError("Mutex::Mutex", error_t);
    }
    
    ///
    /// \brief 直接加锁
    ///
    void Lock() {
        checkError("Mutex::Lock", pthread_mutex_lock(&m_Mutex));
    }
    
    ///
    /// \brief 尝试加锁
    ///
    bool TryLock() {
        int32 error_t = pthread_mutex_trylock(&m_Mutex);
        if (error_t == EBUSY) {
            return false;
        }else{
            checkError("Mutex::trylock", error_t);
            return true;
        }
    }
    
    ///
    /// \brief 解锁
    ///
    void unLock() {
        checkError("Mutex::Unlock", pthread_mutex_unlock(&m_Mutex));
    }
    
private:
    ///
    /// \brief 检查错误的类型
    ///
    static void checkError(const char *error_msg, int error) {
        if (error != 0) {
            string msg = error_msg;
            msg += " error: ";
            msg += strerror(error);
            throw runtime_error(msg);
        }
    }
    
    pthread_mutex_t m_Mutex;
    
    friend class ConditionVariable;
    
private:
    ///
    /// \brief 禁止复制构造
    ///
    Mutex(const Mutex& right);
    
    ///
    /// \brief 禁止赋值
    ///
    Mutex& operator = (const Mutex& right);
    
};

#endif
