#include "sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    sylar::Fiber::ptr cur = sylar::Fiber::GetThis();
    cur->back();
    // sylar::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
    // sylar::Fiber::YieldToHold();
    cur = sylar::Fiber::GetThis();
    cur->back();
}

void test_fiber()
{
    SYLAR_LOG_INFO(g_logger) <<"main begin -1";
    {
        sylar::Fiber::GetThis();
        SYLAR_LOG_INFO(g_logger) << "main begin";
        // 创建 子协程
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber, 0, true));
        // SYLAR_LOG_INFO(g_logger) << "main before swapIn";
        fiber->call();
        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        fiber->call();
        SYLAR_LOG_INFO(g_logger) << "main after end";
        fiber->call();
    }
    SYLAR_LOG_INFO(g_logger) << "main afer end2";
}

int main(int argc, char** argv)
{
    sylar::Thread::SetName("main");
    
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i)
    {
        thrs.push_back(sylar::Thread::ptr(
                        new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    // 指针类型
    for (auto i : thrs)
    {
        i->join();
    }

    return 0;
}