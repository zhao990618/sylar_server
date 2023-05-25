#pragma once

/* 配置系统*/

#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include "log.h"
#include "thread.h"

namespace sylar
{
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;
        ConfigVarBase(const std::string &name, const std::string &description)
            : m_name(name), m_description(description)
        {
            // 将名字中的字母转化为小写字母
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &val) = 0;
        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };

    // 用于基础类型的转化 -- 即int float 等转化为str
    // F from_type ---> T To_type  将F类型转化为T类型
    template <class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                // 若一个node中还存在多个数据，就可以通过递归的方式去存入数据
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                // 若一个node中还存在多个数据，就可以通过递归的方式去存入数据
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                // 若一个node中还存在多个数据，就可以通过递归的方式去存入数据
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                // 若一个node中还存在多个数据，就可以通过递归的方式去存入数据
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    // unordered_set 是 hashset
    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                // 若一个node中还存在多个数据，就可以通过递归的方式去存入数据
                // 因为是 map<std::string, T> 类型的，所以第一个类型需要传入一个std::string类型
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            // v 是map类型的，那么就需要将node以map类型传入
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                // 若一个node中还存在多个数据，就可以通过递归的方式去存入数据
                // 因为是 map<std::string, T> 类型的，所以第一个类型需要传入一个std::string类型
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            // v 是map类型的，那么就需要将node以map类型传入
            YAML::Node node;
            for (auto &i : v)
            {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 传进来是什么类型， 那么就执行什么类型的变量
    /**
     * 设计两个新的类 FromStr 将std::string类型转化class T  (特例化的函数)
     *              ToStr 将class T类型转化为std::string
     * 即重载操作符即可  ---> 重载了 （）
     * class T FromStr::operator()(std::string &)
     * std::string ToStr::operator()(class T&)
     * **/
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef RWMutex RWMutexType;
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

        // 字类构造函数 会先执行父类的构造函数，让父类中的内容先加载
        ConfigVar(const std::string &name, const T &default_value, const std::string &description)
            : ConfigVarBase(name, description), m_val(default_value)
        {
        }
        // 转化为string类型
        std::string toString() override
        {
            try
            {
                RWMutexType::ReadLock lock(m_mutex);
                // 是下 m_val 转化为 std::string
                // return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            }
            catch (const std::exception &e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                                                  << e.what() << "convert: " << typeid(m_val).name() << "to string";
                // std::cerr << e.what() << '\n';
            }
            return "";
        }
        // 从string类型转化为目标类型T
        bool fromString(const std::string &val) override
        {
            try
            {
                // 将std::string 类型的变量转变为 class T类型
                // m_val = boost::lexical_cast<T>(val);
                // FromStr() 是外部类， 若想定义private参数需要内置函数来定义
                setValue(FromStr()(val));
            }
            catch (const std::exception &e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::FromString exception "
                                                  << e.what() << " convert: string to "
                                                  << " name=" << m_name << "\n"
                                                  << val;
            }
            return false;
        }

        // 加上const 则函数体内部的所有参数都是 只读类型， 不可以被修改
        const T getValue()
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }

        void setValue(const T &v)
        {
            {
                // 在作用域内设置读锁，作用域结束后直接析构  --- 如果是写锁的话，有其他线程需要访问m_val会被拒绝
                RWMutexType::ReadLock lock(m_mutex);
                if (v == m_val)
                {
                    return;
                }
                for (auto &i : m_cbs)
                {
                    // function包中 拟定义的函数 on_change_cb(old_value, new_value)
                    i.second(m_val, v);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);
            m_val = v;
        }

        std::string getTypeName() const override { return typeid(T).name(); }

        // 添加监听
        uint64_t addListener(on_change_cb cb)
        {
            /* 函数内部的静态局部变量
                1.只能在定义该变量的函数内使用该变量。2.不论其函数是否被调用，该变量都一直存在。
                3.保留上一次调用后留下的函数值。4.出现在函数内部的基本类型静态变量初始化语句只会执行一次
            */
            static uint64_t s_fun_id = 0;
            // 锁的对象生成，  自动上锁
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        // 删除key所对应的监听量
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        // 得到当前的监听，通过key
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        // 清空监听map
        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

    private:
        RWMutexType m_mutex;
        T m_val;
        // 变更回调函数组， uint64_t作为key，并且要求为一，一般可以用hash值
        std::map<uint64_t, on_change_cb> m_cbs;
    };

    // 管理类
    class Config
    {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType;
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value,
                                                 const std::string &description = "")
        {
            RWMutexType::WriteLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if (it != GetDatas().end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                // 有两种情况，第一种it->second中的类型与已经保存的T符合
                if (tmp)
                {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << "exists";
                    return tmp;
                }
                else
                {
                    // 因为it->second是基本类的智能指针，只能指向基本类，所以需要一个虚函数
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookip name=" << name << "exists but type not "
                                                      << typeid(T).name() << " real_type=" << it->second->getTypeName()
                                                      << " " << it->second->toString();
                    return nullptr;
                }
            }

            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != std::string::npos)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookuo name invalid " << name;
                // 抛出异常参数 name
                throw std::invalid_argument(name);
            }

            // 若找不到 name所对应的ConfigVar对象，则重新生成一个对象 ConfigVar<T>::ptr 为 v
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            // ConfigVar内部存在一个范型T  --- 基本类型ConfigVarBase 强转为 ConfigVar<T>
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &root);
        static ConfigVarBase::ptr LookupBase(const std::string &name);
        // 外部提供function来访问查看 s_datas
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        static ConfigVarMap &GetDatas()
        {
            // 静态的局部成员变量
            static ConfigVarMap s_datas;
            return s_datas;
        }

        static RWMutexType &GetMutex()
        {
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };

}

#endif