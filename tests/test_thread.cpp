#include "sylar.h"
#include <iostream>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
// sylar::RWMutex s_mutex;
sylar::Mutex s_mutex;

void fun1()
{   
    // 静态方法可以被类名 直接调用   ---- 而非静态方法只能被对象所调用
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << sylar::Thread::GetThis()->getId();
    
    for (int i = 0; i < 100000; i++)
    {
        // 生成WriteScopedLockImpl<RWMutex>类型的对象lock,
        // 构造函数的参数是 s_mutex
        // sylar::RWMutex::WriteLock lock(s_mutex);
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
    std::cout << count << std::endl;
}

void fun2()
{

}

int main(int argc, char** argv)
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    // 创建线程池
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 5; i++)
    {
        // 传入函数的引用
        sylar::Thread::ptr thr(new sylar::Thread(&fun1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }
    // std::cout << "1" << std::endl;
    for (int i = 0; i < 5; i++)
    {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << count;
    return 0;
}