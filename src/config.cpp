#include "config.h"
#include <iostream>

namespace sylar
{
    // Config::ConfigVarMap Config::s_datas;
    
    ConfigVarBase::ptr Config::LookupBase(const std::string& name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ?  nullptr : it->second;
    }

    static void ListAllMember(const std::string &prefix, const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        // 如果是非法字符，则输出error文档
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") 
            != std::string::npos)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invaild name: " << prefix << " : " << node;
            return;
        }
        // 如果是合法字符，则需要添加到list中
        output.push_back(std::make_pair(prefix, node));
        // yaml 中带 - 的是squence
        //          不带的是 map
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ListAllMember(prefix.empty() ? it->first.Scalar()
                        :prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node &root)
    {
        // 存储结构数据
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        // 倒入结构数据root，并且保存在all_nodes中
        ListAllMember("", root, all_nodes);

        // 此时 i 是一个对象
        for (auto& i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            // 查看key是否 在初始化的 config中存在，如果有则指向该configVar，并返回其m_var
            ConfigVarBase::ptr var = LookupBase(key);

            if (var)
            {
                // scalar 标量类型 <int || float || class T> 单个数据
                // sequence 队列 <数组> 一组数据
                // map 图表类型<first:second> 一组数据
                if(i.second.IsScalar())
                {
                    var->fromString(i.second.Scalar());
                }
                else{
                    // 如果是sequence即数组类型，可以将数组存入文字流中，再将文字流输入
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }

    }

    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
    {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap& m = GetDatas();
        for (auto it = m.begin(); it != m.end(); it++)
        {
            cb(it->second);
        }
    }

} // namespace sylar
