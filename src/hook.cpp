#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include <dlfcn.h>
namespace sylar
{
    static thread_local bool t_hook_enable = false;
    #define HOOK_FUN(XX)\
        XX(sleep)\
        XX(usleep)\
        XX(nanosleep)\
        XX(socket)\
        XX(connect)\
        XX(accept)\
        XX(fcntl)\
        XX(ioctl)\
        XX(getsockopt)\
        XX(setsockopt)\
        XX(read)\
        XX(readv)\
        XX(recv)\
        XX(recvfrom)\
        XX(recvmsg)\
        XX(write)\
        XX(send)\
        XX(sendto)\
        XX(sendmsg)\
        XX(close)
        

    // 初始化hook
    void hook_init()
    {
        static bool is_inited = false;
        // 查看是否初始化了hook；若没有被初始化就要执行 HOOK_FUN(XX)
        if (is_inited)
        {
            return;
        }
        // ## 表示连接前后两个字符组； # 则是字符描述
        // dlsym 在系统动态库中找到 #name所代表的的函数地址并返回
        // 下面语句中将返回类型强制转化为 (name ## _fun)， 并且复制给了 name ## _f
        // void *dlsym(void *handle, const char *symbol);
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
            HOOK_FUN(XX)
        #undef XX
    }

    //  需要 在main函数执行前，将需要替换的函数全都hook掉，那就只能根据编译顺序来操作
    struct _HookIniter
    {
        // 构造函数
        _HookIniter()
        {
            // 构造函数里面 包含了hook的初始化，
            hook_init();
        }
    };
    
    // 创建一个 静态的变量， 静态的变量在main函数加载之前被先初始化
    static _HookIniter s_hook_initer;

    // 查看是否已经hook了，如果是则返回true；如果不是则返回false；
    bool is_hook_enable()
    {
        return t_hook_enable;
    }
    
    // 设置hook_enbale的状态
    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;

    }

    extern "C"
    {
        // 对HOOK_FUN里面的所有xx(name)宏 都要初始化 函数指针
        #define XX(name) name ## _fun name ## _f = nullptr;
            HOOK_FUN(XX)
        #undef XX

        /*
            实现所有需要hook的函数
        */
       unsigned int sleep(unsigned int seconds)
       {
            // 判断是否被hook了
            if (!sylar::t_hook_enable)
            {
                // 如果没有被hook，那么就要调用原始的函数
                return sleep_f(seconds);
            }
            sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
            sylar::IOManager* iom = sylar::IOManager::GetThis();
            // iom->addTimer(seconds * 1000, std::bind(&sylar::IOManager::schedule, iom, fiber));
            iom->addTimer(seconds * 1000, [iom, fiber](){
               iom->schedule(fiber); 
            });
            sylar::Fiber::YieldToHold();
            return 0;
       }

       int usleep(useconds_t usec)
       {
            // 判断是否被hook了
            if (!sylar::t_hook_enable)
            {
                // 如果没有被hook，那么就要调用原始的函数
                return usleep_f(usec);
            }
            sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
            sylar::IOManager* iom = sylar::IOManager::GetThis();
            // iom->addTimer(seconds / 1000, std::bind(&sylar::IOManager::schedule, iom, fiber));
            iom->addTimer(usec / 1000, [iom, fiber](){
               iom->schedule(fiber); 
            });
            sylar::Fiber::YieldToHold();
            return 0;
       }

       int nanosleep(const struct timespec *req, struct timespec *rem)
       {
            if (!sylar::t_hook_enable)
            {
                return nanosleep_f(req, rem);
            }
            int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
                        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
            sylar::IOManager* iom = sylar::IOManager::GetThis();
            // iom->addTimer(seconds / 1000, std::bind(&sylar::IOManager::schedule, iom, fiber));
            iom->addTimer(timeout_ms, [iom, fiber](){
               iom->schedule(fiber); 
            });
            sylar::Fiber::YieldToHold();
            return 0;
       }
    }
}

