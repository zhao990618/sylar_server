#include "log.h"
#include <iostream>
#include <ctype.h>
#include <map>
#include <functional>
#include <time.h>
#include <stdarg.h>
#include "config.h"

namespace sylar
{
    class Logger;

    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX

        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
        // 该写法可以替换调多个if 或者 switch
#define XX(level, strl)         \
    if (str == #strl)           \
    {                           \
        return LogLevel::level; \
    }
        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);
        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
        return LogLevel::UNKNOW;
#undef XX
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
                      ,const char *file, uint32_t line, uint32_t elapse
                      ,uint32_t threadId, uint32_t fiberId, time_t time
                      ,const std::string& thread_name)
        : m_logger(logger),
          m_level(level),
          m_file(file),
          m_line(line),
          m_elapse(elapse),
          m_threadName(thread_name),
          m_threadId(threadId),
          m_fiberId(fiberId),
          m_time(time)
          
    {
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (-1 != len)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e)
        : m_event(e)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getss();
    }

    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
        if (m_formatter)
        {
            m_hasFormatter = true;
        }
        else
        {
            m_hasFormatter = false;
        }
    }

    LogFormatter::ptr LogAppender::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    Logger::Logger(const std::string &name)
        : m_name(name) // 初始化列表，用于初始化参数
          ,
          m_level(LogLevel::DEBUG)
    {
        // 释放原始m_formatter所只想的空间， 让m_formatter指向新空间 new LogFormatter("")
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        // 初始化 formatter
        if (!appender->getFormatter())
        {
            MutexType::Lock ll(appender->m_mutex);
            appender->m_formatter = m_formatter;
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        // MutexType下的Lock类型 lock
        MutexType::Lock lock(m_mutex);
        // auto 自动类型匹配
        for (auto it = m_appenders.begin();
             it != m_appenders.end(); it++)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppender()
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
        // 当前状况下 appenders中是否有formatter
        for (auto &i : m_appenders)
        {
            // appenders中的其他appender需要上锁免得被修改了
            MutexType::Lock ll(i->m_mutex);
            // 如果appender中没有formater，那么appender->formatter == 外部的formatter
            if (!i->m_hasFormatter)
            {
                i->m_formatter = m_formatter;
            }
        }
    }
    void Logger::setFormatter(const std::string &val)
    {
        // 解析val，判断这个val值是否有格式问题，若有问题(m_error == true)则不将这个值赋值给m_formatter
        sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name = " << m_name << "value = " << val
                      << "invalid formatter" << std::endl;
            return;
        }
        // 若val 没有格式问题，则将val赋值给m_formatter
        setFormatter(new_val);
    }
    LogFormatter::ptr Logger::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {

        if (level >= m_level)
        {
            /*
            若一个类 T 继承 std::enable_shared_from_this ，则会为该类 T 提供成员函数： shared_from_this 。
            当 T 类型对象 t 被一个为名为 pt 的 std::shared_ptr 类对象管理时，
            调用 T::shared_from_this 成员函数，将会返回一个新的 std::shared_ptr 对象。
            */
            auto self = shared_from_this();
            MutexType::Lock lock(m_mutex);
            if (!m_appenders.empty())
            {
                // 自动类型指针指向m_appenders，所以i的类型时LogAppender
                for (auto &i : m_appenders)
                {
                    // 调用了LogAppender中的log函数，该函数将会执行 日志的输出
                    i->log(self, level, event);
                }
            }
            else if (m_root)
            {
                m_root->log(level, event);
            }
        }
    }

    std::string Logger::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }

        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }

    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            MutexType::Lock lock(m_mutex);
            std::string str = m_formatter->format(logger, level, event);
            std::cout << str;
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &filename)
        : m_filename(filename)
    {
        reopen();
    }

    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            // 当前时间
            uint64_t now = time(0);
            if (now != m_lastTime)
            {
                reopen();
                m_lastTime = now;
            }
            MutexType::Lock lock(m_mutex);
            // reopen();
            // 从文件中输出
            if (!(m_filestream << m_formatter->format(logger, level, event)))
            {
                std::cout << "error" << std::endl;
            }
        }
    }

    bool FileLogAppender::reopen()
    {
        MutexType::Lock lock(m_mutex);
        // 判断文件流的状态，来观察文件是关闭还是打开
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        // !!的作用是 非0转化为1,0还是0
        return !!m_filestream;
    }

    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };
    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLogger()->getName();
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };
    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
    };
    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };
    class DataTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DataTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M%S")
            : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            // 得到当前时间放入在time中，但是tm中作为备份，用于操作
            localtime_r(&time, &tm);
            char buf[64];
            // m_format 是string类型，c_str C++的string转化为c类型的string数组返回头指针
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };
    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };
    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };
    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };
    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str)
            : m_string(str) {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    // Tab 键位的操作
    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }

    private:
        std::string m_string;
    };

    LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    void LogFormatter::init()
    {
        // str, format, type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        // size_t last_pos = 0;

        for (size_t i = 0; i < m_pattern.size(); i++)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while (n < m_pattern.size())
            {
                // 检测 传入参数 是否为 字母表内的字母,也不是 {}
                if (!fmt_status && (!isalpha(m_pattern[n]) && '{' != m_pattern[n] && '}' != m_pattern[n]))
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        // 获取%后的第一个值 %m{xxxx}的形式
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; // 解析格式
                        fmt_begin = n;  // 表示{的位置
                        n++;
                        continue;
                    }
                }

                if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        // 截取 {} 中间的字符
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }

                n++;
                // 如果n到最后 str也为空，即整个pattern中没有{}来分割内容
                if (n == m_pattern.size())
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }
            if (fmt_status == 0)
            {
                // 如果没有 {} 的情况下
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                // 直接把 % 下的所有 str 提取出来，放在
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "Pattern pates error " << m_pattern << "-" << m_pattern.substr(i) << std::endl;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
                m_error = true;
                i = n - 1;
            }
        }
        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        // map<string, function<string>> s_format_items
        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
        // str -> string, FormatItem::ptr(new C(fmt)); -> function<str>
#define XX(str, C)                                                               \
    {                                                                            \
#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),       // m:消息
            XX(p, LevelFormatItem),         // p:日志级别
            XX(r, ElapseFormatItem),        // r:累计毫秒数
            XX(c, NameFormatItem),          // c:日志名称
            XX(t, ThreadIdFormatItem),      // t:线程ID
            XX(n, NewLineFormatItem),       // n:换行符
            XX(d, DataTimeFormatItem),      // d:时间
            XX(f, FilenameFormatItem),      // f:文件名称
            XX(l, LineFormatItem),          // l:行数
            XX(T, TabFormatItem),           // T:Tab
            XX(F, FiberIdFormatItem),       // F:协程ID
            XX(N, ThreadNameFormatItem),    // N:线程名称

#undef XX
        };

        for (auto &i : vec)
        {
            // 得到vec中保存的 第3个值
            if (std::get<2>(i) == 0)
            {
                //  m_items 的存储类型是 FormatItem::ptr
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                // 找到对应的std::get<0>(i)值 it --- 如果get<0>(i)这个是字母，那么就可以从s_format_items找到相对应的类方法初始化
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    // 没找到
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    // 找到了对应的值， second就是function<>  把fmt作为重要参数压入栈中
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // std::cout << "{" << std::get<0>(i) << "} - {" << std::get<1>(i) << "} - {" << std::get<2>(i) << "}" << std::endl;
        }
        // std::cout << m_items.size() << std::endl;
    }

    LoggerManager::LoggerManager()
    {
        // 类的构造函数 让 m_root 指向新的对象
        m_root.reset(new sylar::Logger());
        // 初始化该新的对象的Appender -- 终端输出
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        // 将m_root添加到m_loggers
        m_loggers[m_root->getName()] = m_root;
        init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        MutexType::Lock lock(m_mutex);
        // 如果找得到，那么就返回这个name所对应的Logger
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }
        // 如果找不到，则返回一个默认类型的 Logger::ptr (m_root)
        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root; // m_root 是Logger::ptr 智能指针
        m_loggers[name] = logger;
        return logger;
    }

    struct LogAppenderDefine
    {
        int type = 0; // 1.File, 2.Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const
        {
            return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
        }
    };

    // 日志类型定义  -- 出现那种格式的日志，每一种日志都存放在 LogDefine中
    struct LogDefine
    {
        std::string name;                         // 日志名称
        LogLevel::Level level = LogLevel::UNKNOW; // 日志级别
        std::string formatter;
        std::vector<LogAppenderDefine> appenders; // Appender集合

        // 添加了const修饰，说明该函数可以被const成员变量和非const成员变量调用
        // 不添加const修饰，说明该函数只能被 非const成员变量调用
        bool operator==(const LogDefine &oth) const
        {
            return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
        }

        bool operator<(const LogDefine &oth) const
        {
            return name < oth.name;
        }
    };

    // 需要片特化 ----
    /* 若 template<class T> 那么实现片特化 必须要有 class T 的参加*/
    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &v)
        {
            // 将string类型转化为Node类型， 主要是数组结构
            YAML::Node n = YAML::Load(v);

            LogDefine ld;
            // 判断n中是否有name这个参数
            if (!n["name"].IsDefined())
            {
                std::cout << "log config error: name is null, " << n << std::endl;
                throw std::logic_error("log config name is null");
            }

            // 添加name参数
            ld.name = n["name"].as<std::string>();
            // 添加level参数 -- 将string传入
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if (n["formatter"].IsDefined())
            {
                ld.formatter = n["formatter"].as<std::string>();
            }

            // 添加 appender内的参数
            if (n["appenders"].IsDefined())
            {
                for (size_t x = 0; x < n["appenders"].size(); x++)
                {
                    auto a = n["appenders"][x];
                    if (!a["type"].IsDefined())
                    {
                        std::cout << "log config error: appender type is null, " << a << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    // 创建 LogAppenderDefine 对象
                    LogAppenderDefine lad;
                    if (type == "FileLogAppender")
                    {
                        lad.type = 1;
                        // 添加appender --> file路径
                        if (!a["file"].IsDefined())
                        {
                            std::cout << "log config error: fileappender type is null, " << a << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        // 添加appender --> formatter
                        if (a["formatter"].IsDefined())
                        {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    }
                    else if (type == "StdoutLogAppender")
                    {
                        lad.type = 2;
                        // 添加appender --> formatter
                        if (a["formatter"].IsDefined())
                        {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    }
                    else
                    {
                        std::cout << "log config error: appender type is null, " << a << std::endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            }

            return ld;
        }
    };

    template <>
    class LexicalCast<LogDefine, std::string>
    {
        // yaml 中带 - 的是数组类型需要用push_back, 其他的是用[""] = xxx
    public:
        std::string operator()(const LogDefine &i)
        {
            YAML::Node n;
            n["name"] = i.name;
            if (i.level != LogLevel::UNKNOW)
            {
                n["level"] = sylar::LogLevel::ToString(i.level);
            }
            if (!i.formatter.empty())
            {
                n["formatter"] = i.formatter;
            }
            for (auto &a : i.appenders)
            {
                YAML::Node na;
                if (1 == a.type)
                {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                }
                else if (2 == a.type)
                {
                    na["type"] = "StdoutLogAppender";
                }
                if (!a.formatter.empty())
                {
                    na["formatter"] = a.formatter;
                }
                if (a.level != sylar::LogLevel::UNKNOW)
                {
                    na["level"] = LogLevel::ToString(a.level);
                }
                n["appenders"].push_back(na);
            }

            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };

    // 返回的 是ptr类型
    sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
        sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    // 全局对象 会在main函数之前执行
    struct LogIniter
    {
        LogIniter()
        {
            // 属性 出现变化时，一般是以下几种情况
            g_log_defines->addListener([](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value)
                                       {
                SYLAR_LOG_INFO(SYLAR_LOG_NAME(SYLAR_LOG_ROOT()->getName())) << "on_logger_conf_changed";
                // 新增
                for (auto& i : new_value)
                {
                    auto it = old_value.find(i);
                    sylar::Logger::ptr logger;
                    if (it == old_value.end())
                    {
                        // 新增logger    ---- 初始化的 formatter 是默认的
                        logger = SYLAR_LOG_NAME(i.name); 
                    }
                    else
                    {
                        if(!(i == *it))
                        {
                            // 修改，出现变化的logger
                            logger = SYLAR_LOG_NAME(i.name);
                        }
                    }
                    logger->setLevel(i.level);
                    // 设置 formatter
                    if (!i.formatter.empty())
                    {
                        logger->setFormatter(i.formatter);
                    }

                    // 清空appender， 填充新的appender值
                    logger->clearAppender();
                    for (auto& a : i.appenders)
                    {
                        // 添加新的appender
                        sylar::LogAppender::ptr ap;
                        if (a.type == 1)
                        {
                            // 文件输出格式
                            ap.reset(new sylar::FileLogAppender(a.file));
                        }
                        else if (a.type == 2)
                        {
                            // 控制台输出格式
                            ap.reset(new sylar::StdoutLogAppender());
                        }
                        ap->setLevel(a.level);

                        if (!a.formatter.empty())
                        {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if (!fmt->isError())
                            {
                                ap->setFormatter(fmt);
                            }
                            else
                            {
                                std::cout << "log.name=" << i.name<<" appender type=" << a.type 
                                          << " formatter=" << a.formatter << " is invalid" << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }

                // 删除  -- old_vlaue 里面有， new_value 没有
                for (auto& i : old_value)
                {
                    auto it = new_value.find(i);
                    if (it == new_value.end())
                    {
                        // 删除logger
                        auto logger = SYLAR_LOG_NAME(i.name);
                        // 设置level是一个非常高的等级（不存在的等级），相当于将这个logger关闭了
                        logger->setLevel((LogLevel::Level)100);
                        // 清空
                        logger->clearAppender();
                    }
                } });
        }
    };

    // 这个定义在main函数之前
    static LogIniter __log_init;

    std::string LoggerManager::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for (auto &i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void LoggerManager::init()
    {
    }
}