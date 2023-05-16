#include <iostream>
#include <pthread.h>
#include "log.h"
#include "util.h"

int main(int argc, char** argv)
{
    sylar::Logger::ptr logger(new sylar::Logger());
    // 先设置 输出到 --控制台 -- （匿名指针）
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender()));
    // 指针名为 -- file_appender -- ( 输出到文件中 需要文件目录设置 )
    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("/home/zhaoyangfan/LinuxStudio/server/file_Log/log.txt"));
    
    // 自定义 设置 文件格式
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);

    logger->addAppender(file_appender);

    /*
    // 设置事件文档内容
    sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)));
    
    // 把字符传传给 字符流 ss
    event->getss() << "hello world";

    // 添加该事件从
    logger->log(sylar::LogLevel::DEBUG, event);
    */
    
    std::cout << "hello sylar log" << std::endl;
    
    SYLAR_LOG_INFO(logger) << "test macro info";
    SYLAR_LOG_ERROR(logger) << "test macro error";

    SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    
    // 返回了 LoggerManager 类型指针
    auto l = sylar::loggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";

    return 0;
}