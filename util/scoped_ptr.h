/*!
 * \file scoped_ptr.h
 * \brief 资源管理类
 *
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */

#ifndef DCR_scoped_ptr_h
#define DCR_scoped_ptr_h

#include <cassert>
#include <cstdlib>

template <class T> class scoped_ptr;

///
/// \brief 资源管理类
///
template <class T>
class scoped_ptr {
public:
    
    typedef T element_type;
    
    explicit scoped_ptr(T *p = NULL):m_ptr(p) {}
    
    ///
    /// \brief 析构函数要防止删除不完整类型
    ///
    ~scoped_ptr() {
        enum {
            type_must_be_complete = sizeof(T)
        };
        delete m_ptr;
    }
    
    ///
    /// \brief 重置指针
    ///
    void reset(T *p = NULL) {
        if (p != m_ptr) {
            enum {
                type_must_be_complete = sizeof(T)
            };
            delete m_ptr;
            m_ptr = p;
        }
    }
    
    ///
    /// \brief 重载*运算符
    ///
    T& operator *() const {
        assert(m_ptr != NULL);
        return *m_ptr;
    }
    
    ///
    /// \brief 重载->运算符
    ///
    T* operator->() const {
        assert(m_ptr != NULL);
        return m_ptr;
    }
    
    T* get() const{
        return m_ptr;
    }
    
    ///
    /// \brief 重载==运算符
    ///
    bool operator==(T *p)const {
        return m_ptr == p;
    }
    
    ///
    /// \brief 重载!=运算符
    ///
    bool operator!=(T *p)const {
        return m_ptr != p;
    }
    
    ///
    /// \brief 交换两个资源管理类的
    ///

    void swap(scoped_ptr& p2) {
        T* tmp = m_ptr;
        m_ptr = p2.m_ptr;
        p2.m_ptr = tmp;
    }
    
    ///
    /// \brief 资源管理类放弃资源
    ///
    T* release() {
        T* retVal = m_ptr;
        m_ptr = NULL;
        return retVal;
    }
    
    
private:
    T *m_ptr;   ///<  资源指针
    
    ///
    /// \brief 资源管理类禁止比较
    ///
    template <class C2>
    bool operator==(scoped_ptr<C2> const& p2) const;
    
    ///
    /// \brief 资源管理类禁止比较
    ///
    template <class C2>
    bool operator!=(scoped_ptr<C2> const& p2) const;
    
    ///
    /// \brief 禁止复制构造
    ///
    scoped_ptr(const scoped_ptr&);
    
    ///
    /// \brief 禁止赋值
    ///
    void operator=(const scoped_ptr&);
};

#endif
