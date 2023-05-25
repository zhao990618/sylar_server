#pragma once
/* 日志系统 */
#ifndef __SYLAR_LOG_H__
#define __STLAR_LOG_H__
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

/*
    利用局部变量的生命周期实现了操作，SYLAR_LOG_LEVEL宏在if语句内部构造了局部变量
    LogEventWrap,LogEventWrap在if语句结束后会结束生命周期，调用析构函数实现操作
    sylar::LogEventWrap() 是一个对象不是智能指针
*/

// 宏定义 是在预编译期间的，是全局类型的

#define SYLAR_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(\
        logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)\
    if (logger->getLevel() <= level)\
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(\
        logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getEvent()->format(fmt, ##__VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, ##__VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::loggerMgr::GetInstance()->getRoot()
// 找到key 为name的 logger
#define SYLAR_LOG_NAME(name) sylar::loggerMgr::GetInstance()->getLogger(name)

namespace sylar
{
    class Logger;
    class LoggerManager;

    // 日志级别
    class LogLevel
    {
    public:
        // 日志级别
        enum Level
        {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };
        static const char* ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string& str);
    };

    // 日志事件
    class LogEvent
    {
    public:
        // 定义指针类型
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
                ,const char* file, uint32_t line, uint32_t elapse
                ,uint32_t threadId,uint32_t fiberId, time_t time
                ,const std::string& thread_name);


        const char* getFile() const {return m_file;}
        uint32_t getLine()  const {return m_line;}
        uint32_t getElapse()  const {return m_elapse;}
        uint32_t getThreadId()  const {return m_threadId;}
        uint32_t getFiberId()  const {return m_fiberId;}
        time_t getTime() const {return m_time;}
        std::string getContent() const {return m_ss.str();}
        std::shared_ptr<Logger> getLogger() const {return m_logger;}
        LogLevel::Level getLevel() const {return m_level;}
        const std::string& getThreadName() const {return m_threadName;}
        std::stringstream& getss() {return m_ss;}

        void format(const char * fmt, ...);
        void format(const char * fmt, va_list al);
    private:
        // 成员变量前面 加 m_ 表示成员变量，用来和方法参数名称 所区分开
        const char *m_file = nullptr; // 文件名
        uint32_t m_line = 0;          // 行号
        uint32_t m_elapse = 0;        // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;      // 线程 id
        uint32_t m_fiberId = 0;       // 协程 id
        time_t m_time;                // 时间戳
        std::stringstream m_ss;        //
        std::string m_threadName;       // 线程名字  
        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };

    class LogEventWrap
    {
        public:
            LogEventWrap(LogEvent::ptr e);
            ~LogEventWrap();

            std::stringstream& getSS();
            LogEvent::ptr getEvent() const {return m_event;};
        private:
            LogEvent::ptr m_event;
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        // 传入一个 pattern模式 --- 字符类型模式
        LogFormatter(const std::string &pattern);

        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    // private:
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            FormatItem(const std::string& fmt = ""){}
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };
        // 日志格式的解析
        void init();

        bool isError() const {return m_error;}
        const std::string getPattern() const {return m_pattern;}
    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        /// 是否有错误  --- 即formatter 是错误类型的，所以该formatter所对应的logger是错误的 
        bool m_error = false;
    };

    // 日志输出点
    class LogAppender
    {
    friend class Logger;
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef SpinLock MutexType;
        // 虚函数，即抽象
        virtual ~LogAppender() {}

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0; 
        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();

        void setLevel(LogLevel::Level val){m_level = val;}
        LogLevel::Level getLevel() const {return m_level;} 
        
    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        bool m_hasFormatter = false;
        MutexType m_mutex;
        LogFormatter::ptr m_formatter;
    };

    // 日志输出器
    class Logger : public std::enable_shared_from_this<Logger>
    {
    friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef SpinLock MutexType;

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppender();

        LogLevel::Level getLevel() const
        {
            return m_level;
        }
        void setLevel(LogLevel::Level val)
        {
            m_level = val;
        }

        const std::string& getName() const {return m_name;}
        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string& val);
        LogFormatter::ptr getFormatter();

        std::string toYamlString();

    private:
        std::string m_name;                      // 日志名称
        LogLevel::Level m_level;                 // 日志级别
        std::list<LogAppender::ptr> m_appenders; // Appender集合
        LogFormatter::ptr m_formatter;
        MutexType m_mutex;
        Logger::ptr m_root;
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        // override 证明该函数 是从虚类中继承出来的函数
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
    };

    // 定义输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:

        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        // override 证明该函数 是从虚类中继承出来的函数
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        // 重新打开文件，文件打开成功为ture ，否则为faile
        bool reopen();
        std::string toYamlString() override;
    private:
        std::string m_filename;
        std::ofstream m_filestream;
        uint64_t m_lastTime = 0;
    };

    // log管理器 --- 产生logger
    class LoggerManager
    {
    public:
        // typedef Mutex MutexType;
        typedef SpinLock MutexType;
        // 无参数的默认构造函数  --- m_root指向一个新的Logger 获得默认初始化 StdoutLogAppender
        LoggerManager();
        Logger::ptr getLogger(const std::string& name);

        void init();
        Logger::ptr getRoot() const {return m_root;}
        std::string toYamlString();
    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    // 单例 变量 -- 一个LoggerManager 类型的对象
    typedef sylar::Singleton<sylar::LoggerManager> loggerMgr;

} // namespace sylar

#endif