#include "sylar.h"
#include <iostream>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test1()
{
    SYLAR_LOG_INFO(g_logger) << "test1";

    static int s_count = 5;
    sleep(1);
    while(--s_count >= 0)
    {
        sylar::Scheduler::GetThis()->schedule(&test1);
    }
}

int main(int argc, char** argv)
{

    SYLAR_LOG_INFO(g_logger) << "main";
    // sylar::Scheduler sc(3, false, "Test"); // 主线程只会执行5次 schedule(&test1);，其他3个线程结束后再终止当前线程
    sylar::Scheduler sc(3, true, "Test");
    sc.start();
    sc.schedule(&test1);
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "end";

    // sylar::Scheduler::ptr sc_ptr(new sylar::Scheduler(1, true, "1"));
    // sc_ptr->stop();


    return 0;
}