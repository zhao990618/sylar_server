#include "log.h"
#include "config.h"
#include "string"
#include <iostream>
#include <yaml-cpp/yaml.h>

#if 0
sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port", (int)8080, " :system port");

sylar::ConfigVar<float>::ptr g_int_values_config =
    sylar::Config::Lookup("system.port", (float)8080.2f, " :system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)12.7f, " :system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    sylar::Config::Lookup("system.vec_int", std::vector<int>{1, 2}, " :system int vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    sylar::Config::Lookup("system.list_int", std::list<int>{1, 2}, " :system int list");

sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    sylar::Config::Lookup("system.set_int", std::set<int>{1, 2}, " :system int set");

sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    sylar::Config::Lookup("system.uset_int", std::unordered_set<int>{1, 2}, " :system int uset");

sylar::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    sylar::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, " :system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config =
    sylar::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 2}}, " :system str int umap");

/**
 * node.Type() 返回的是当前YAML::Node自定义的类型  --
 **/
void printf_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                                         << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull())
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                                         << "Null - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        // map<first, second>  ---> second(node)
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            // map中的first是当前文本，second是余下的其他内容
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                                             << it->first << " - " << it->second.Type() << " - " << level;
            printf_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                                             << i << " - " << node[i].Type() << " - " << level;

            printf_yaml(node[i], level + 1);
        }
    }
}

// 测试加载yaml
void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/zhaoyangfan/LinuxStudio/server/file_yaml/log.yaml");
    printf_yaml(root, 0);
}

// 测试 config
void test_config()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before:" << g_int_value_config->getValue();   // 返回的是类型T
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before:" << g_float_value_config->toString(); // 返回的是类型std::string

// 宏定义  -- 宏函数
#define XX(g_var, name, prefix)                                                               \
    {                                                                                         \
        auto &v = g_var->getValue();                                                          \
        for (auto &i : v)                                                                     \
        {                                                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i;                  \
        }                                                                                     \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

// 宏定义  -- 宏函数
#define XX_M(g_var, name, prefix)                                                             \
    {                                                                                         \
        auto &v = g_var->getValue();                                                          \
        for (auto &i : v)                                                                     \
        {                                                                                     \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": {" << i.first            \
                                             << " - " << i.second << "}";                     \
        }                                                                                     \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

    XX(g_int_vec_value_config, vec_int, before);
    XX(g_int_list_value_config, list_int, before);
    XX(g_int_set_value_config, set_int, before);
    XX(g_int_uset_value_config, uset_int, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before" << g_int_value_config->getName() << "\t"
    //                                     << g_int_value_config->getDescription();

    YAML::Node root = YAML::LoadFile("/home/zhaoyangfan/LinuxStudio/server/file_yaml/log.yaml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after:" << g_float_value_config->toString();

    XX(g_int_vec_value_config, vec_int, after);
    XX(g_int_list_value_config, list_int, after);
    XX(g_int_set_value_config, set_int, after);
    XX(g_int_uset_value_config, uset_int, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}
#endif
/**
 * 自定义类型的测试
 * **/

class Person
{
public:
    std::string m_name = "";
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex << "]";
        return ss.str();
    }

    bool operator==(const Person &oth) const
    {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};

// 片特化
namespace sylar
{
    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

}

// test
sylar::ConfigVar<Person>::ptr ptr_person =
    sylar::Config::Lookup("class.person", Person(), "system Person");

void test_class()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << ptr_person->getValue().toString()
                                     << " - " << ptr_person->toString();

    // 监听是否出现 参数值的变化
    ptr_person->addListener([](const Person &old_value, const Person &new_value)
                            { SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value = " << old_value.toString() << "\t" 
                                                               << "new_value = " << new_value.toString();});

    YAML::Node root = YAML::LoadFile("/home/zhaoyangfan/LinuxStudio/server/file_yaml/log.yaml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << ptr_person->getValue().toString()
                                     << " - " << ptr_person->toString();
}

void test_log()
{   
    static sylar::Logger::ptr system_log = SYLAR_LOG_NAME("system");
    SYLAR_LOG_INFO(system_log) << "hello ststem" << std::endl;
    std::cout << sylar::loggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/zhaoyangfan/LinuxStudio/server/file_yaml/log.yaml");
    sylar::Config::LoadFromYaml(root);

    std::cout << "=============" << std::endl;
    std::cout << sylar::loggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    SYLAR_LOG_INFO(system_log) << "hello ststem" << std::endl;
    // 自定义类型
    system_log->setFormatter("%d-[%p]-%m%n");
    SYLAR_LOG_INFO(system_log) << "hello ststem11" << std::endl;
}

int main(int agrc, char **argv)
{
    // test_yaml();
    // test_config();
    // test_class();
    test_log();

    sylar::Config::Visit([](sylar::ConfigVarBase::ptr var)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "name=" << var->getName()
                                             << " descripation=" << var->getDescription()
                                             << " typename=" << var->getTypeName()
                                             << " value=" << var->toString();
        }
    );

    return 0;
}