/*!
 * \file class_register.h
 * \brief 用户派生类的注册机制
 *
 * 主要原理是通过用户派生类的名字的字符串和用户派生类创建函数的函数指针用一个Map进行保存，
 * 通过字符串就可以创建对应的类，和获取对应的大小
 *
 * \author Jiang Jiazhi
 * \version 1.0
 * \date 2014.9.8
 */


#ifndef DCR_class_register_h
#define DCR_class_register_h

#include <map>
#include <string>
#include "../util/common.h"

//ObjectCreatorRegisterAction_##register_name这个类在构造的时候注册向注册中心注册当前类
#define DECLARE_CLASS_REGISTER_CENTER(register_name, base_class_name)                               \
    class ObjectCreatorRegisterCenter_##register_name {                                             \
    public:                                                                                         \
        typedef base_class_name* (*creator_t)();                                                    \
                                                                                                    \
        ObjectCreatorRegisterCenter_##register_name() {}                                            \
        ~ObjectCreatorRegisterCenter_##register_name() {}                                           \
                                                                                                    \
        void addCreator(std::string entry_name, creator_t creator) {                                \
            m_creator_registy[entry_name] = creator;                                                \
        }                                                                                           \
                                                                                                    \
        void addObjectSize(std::string entry_name, int32 object_size) {                             \
            m_object_registy[entry_name] = object_size;                                             \
        }                                                                                           \
                                                                                                    \
        base_class_name* createObject(const std::string& entry_name);                               \
                                                                                                    \
        int32 getObjectSize(const std::string& entry_name);                                         \
                                                                                                    \
    private:                                                                                        \
        typedef std::map<std::string, creator_t> creator_registy_t;                                 \
        typedef std::map<std::string, int32> object_size_registy_t;                                 \
        creator_registy_t m_creator_registy;                                                        \
        object_size_registy_t m_object_registy;                                                     \
    };                                                                                              \
                                                                                                    \
                                                                                                    \
    inline ObjectCreatorRegisterCenter_##register_name&                                             \
    getRegisterCenter_##register_name() {                                                           \
        static ObjectCreatorRegisterCenter_##register_name register_center;                         \
        return register_center;                                                                     \
    }                                                                                               \
                                                                                                    \
                                                                                                    \
    class ObjectCreatorRegisterAction_##register_name {                                             \
    public:                                                                                         \
        ObjectCreatorRegisterAction_##register_name(                                                \
            const std::string& entry_name,                                                          \
            ObjectCreatorRegisterCenter_##register_name::creator_t creator,                         \
            int32 object_size) {                                                                    \
            getRegisterCenter_##register_name().addCreator(entry_name, creator);                    \
            getRegisterCenter_##register_name().addObjectSize(entry_name, object_size);             \
        }                                                                                           \
                                                                                                    \
        ~ObjectCreatorRegisterAction_##register_name() {}                                           \
    };                                                                                              \




#define IMPLEMENT_CLASS_REGISTER_CENTER(register_name, base_class_name)                             \
    base_class_name* ObjectCreatorRegisterCenter_##register_name::createObject(                     \
        const std::string& entry_name) {                                                            \
                                                                                                    \
        creator_t creator = NULL;                                                                   \
        creator_registy_t::const_iterator it =                                                      \
            m_creator_registy.find(entry_name);                                                     \
        if(it != m_creator_registy.end()) {                                                         \
            creator = it->second;                                                                   \
        }                                                                                           \
                                                                                                    \
        if(creator != NULL) {                                                                       \
            return (*creator)();                                                                    \
        }else {                                                                                     \
            return NULL;                                                                            \
        }                                                                                           \
    }                                                                                               \
                                                                                                    \
    int32 ObjectCreatorRegisterCenter_##register_name::getObjectSize(                               \
        const std::string& entry_name) {                                                            \
                                                                                                    \
        int32 object_size = -1;                                                                     \
        object_size_registy_t::const_iterator it =                                                  \
            m_object_registy.find(entry_name);                                                      \
        if(it != m_object_registy.end()) {                                                          \
            object_size = it->second;                                                               \
        } else {                                                                                    \
            cout << "map size: " << m_object_registy.size();                                        \
            cout << "not find " << entry_name << endl;                                              \
        }                                                                                           \
                                                                                                    \
        return object_size;                                                                         \
    }                                                                                               \



//第一段为定义一个构造class_name的函数
//第二段为调用ObjectCreatorRegisterAction_##register_name的构造函数
#define CLASS_OBJECT_CREATOR_REGISTER_ACTION(register_name,                                         \
                                             base_class_name,                                       \
                                             entry_name_as_string,                                  \
                                             class_name)                                            \
    base_class_name* objectCreator_##register_name##class_name() {                                  \
        return new class_name;                                                                      \
    }                                                                                               \
    ObjectCreatorRegisterAction_##register_name                                                     \
    g_object_creator_register_##register_name##class_name(                                          \
        entry_name_as_string,                                                                       \
        objectCreator_##register_name##class_name, sizeof(class_name))                              \




#define CLASS_CREATE_OBJECT(register_name, entry_name_as_string)                                    \
        getRegisterCenter_##register_name().createObject(entry_name_as_string)                      \


#define GET_OBJECT_SIZE(register_name, entry_name_as_string)                                        \
        getRegisterCenter_##register_name().getObjectSize(entry_name_as_string)                     \

#endif
