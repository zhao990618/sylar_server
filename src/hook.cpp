#include "hook.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include <dlfcn.h>
namespace sylar
{
    sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
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

    struct timer_info
    {
        int cancelled = 0;
    };
    
    template<typename OriginFun, typename ... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                        uint32_t event, int timeout_so, Args&&... args)
    {
        // 看看是否hook了
        if (!sylar::t_hook_enable)
        {
            // forward 将参数完美传递给fun，forward用&&既可以传递了右值(无命名的)和左值(有命名的)
            return fun(fd, std::forward<Args>(args)...)
        }

        // 获得当前fd是否是socket的并且保存在m_datas中
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
        // 如果fd不存在于m_datas中,那么就被视为 非socket类型的fd
        if (!ctx)
        {
            return fun(fd, std::forward<Args>(args)...);
        }
        // 如果存在于m_datas中，并且 已经 关闭了
        if (ctx->close())
        {
            errno = EBADF;
            return -1;
        }
        // 如果不是socket，并且用户设置了这个fd为非阻塞
        if (!ctx->isSocket() || ctx->getUserNonblock())
        {
            return fun(fd, std::forward<Args>(args)...);
        }
        // 得到ctx的超时时间
        uint64_t to = ctx->getTimeout(timeout_so);
        // 条件状态， 用于 conditionEvent
        std::shared_ptr<timer_info> tinfo(new timer_info);

    retry:
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        // 如果n = -1,并且errno == EINTR，说明函数被中断异常退出
        while(n == -1 && errno == EINTR)
        {
            n = fun(fd, std::forward<Args>(args)...);
        }
        // 如果n = -1,并且errno == EAGAIN，说明fd被阻塞了
        if (n == -1 && erron == EAGAIN)
        {
            sylar::IOManager* iom = sylar::IOManager::GetThis();
            sylar::Timer::ptr timer;
            // weak_ptr指针指向 timer_info类型的对象tinfo 
            std::weak_ptr<timer_info> winfo(tinfo);
            // to 来自于 ctx->getTimeout(timeout_so);
            if (to != (uint64_t)-1)
            {
                // to 不等于-1，那么就说明有超时时间 (在超时时间内完成操作就不会被cancel)
                timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
                    // lock 返回的是一个智能指针
                    auto t = winfo.lock();
                    // 如果t不存在并且t被取消了，那么就直接不执行这个函数
                    if (!t || t->cancelled)
                    {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    // 把当前iomanager事件取消掉  --- 有triggerEvent来添加schedul
                    iom->cancelEvent(fd, (sylar::IOManager::Event)event);
                }, winfo)
            }
            int c = 0;
            uint64_t now = 0;
            // 添加时间到timer中  --- 有triggerEvent来添加schedul
            int rt = iom->addEvent(fd, (sylar::IOManager::Event)event);
            // rt为0表示成功添加event，如果进入下面的if那么就是add失败，就要取消定时器
            if (rt)
            {
                if (c)
                {
                    SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                        << fd < ", " << event << ") retry c=" << c
                        << " used=" << (sylar::GetCurrentUS() - now);
                }
                // 如果timer此时没有超时，那么timer必定存在
                if (timer)
                {
                    timer->cancel();
                }
                return -1;
            }
            else
            {
                // event添加成功
                sylar::Fiber::YieldToHold();
                // 如果fiber 被唤醒回来timer还存在的话，那就取消掉timer
                if (timer)
                {
                    timer->cancel();
                }
                // 时间没有被执行失败
                if (tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }

                // 如果 数据没有被读取完毕那就重新返回retry，继续去读取数据
                // 当前结构是如果读取被阻塞了
                goto retry;
            }
        }
        return n;
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

