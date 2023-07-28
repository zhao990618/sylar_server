
#include "hook.h"
#include <dlfcn.h>
#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "macro.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
namespace sylar
{
    static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
        sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
    static thread_local bool t_hook_enable = false;
#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)   \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
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
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX)
#undef XX
    }

    static uint64_t s_connect_timout = -1;
    //  需要 在main函数执行前，将需要替换的函数全都hook掉，那就只能根据编译顺序来操作
    struct _HookIniter
    {
        // 构造函数
        _HookIniter()
        {
            // 构造函数里面 包含了hook的初始化，
            hook_init();
            s_connect_timout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int &old_value, const int &new_value)
                                               {
                SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from"
                                         <<  old_value << " to " << new_value;
                s_connect_timout = new_value; });
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

    template <typename OriginFun, typename... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                         uint32_t event, int timeout_so, Args &&...args)
    {
        // 看看是否hook了
        if (!sylar::t_hook_enable)
        {
            // forward 将参数完美传递给fun，forward用&&既可以传递了右值(无命名的)和左值(有命名的)
            return fun(fd, std::forward<Args>(args)...);
        }

        // 获得当前fd是否是socket的并且保存在m_datas中
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
        // 如果fd不存在于m_datas中,那么就被视为 非socket类型的fd
        if (!ctx)
        {
            return fun(fd, std::forward<Args>(args)...);
        }
        // 如果存在于m_datas中，并且 已经 关闭了
        if (ctx->isClose())
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
        while (n == -1 && errno == EINTR)
        {
            n = fun(fd, std::forward<Args>(args)...);
        }
        // SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
        // 如果n = -1,并且errno == EAGAIN，说明fd被阻塞了
        if (n == -1 && errno == EAGAIN)
        {
            sylar::IOManager *iom = sylar::IOManager::GetThis();
            sylar::Timer::ptr timer;
            // weak_ptr指针指向 timer_info类型的对象tinfo
            std::weak_ptr<timer_info> winfo(tinfo);
            // to 来自于 ctx->getTimeout(timeout_so);
            if (to != (uint64_t)-1)
            {
                // to 不等于-1，那么就说明有超时时间 (在超时时间内完成操作就不会被cancel)
                timer = iom->addConditionTimer(
                    to, [winfo, fd, iom, event]()
                    {
                    // lock 返回的是一个智能指针
                    auto t = winfo.lock();
                    // 如果t不存在并且t被取消了，那么就直接不执行这个函数
                    if (!t || t->cancelled)
                    {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    // 把当前iomanager事件取消掉  --- 有triggerEvent来添加schedul
                    iom->cancelEvent(fd, (sylar::IOManager::Event)event); },
                    winfo);
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
                                              << fd << ", " << event << ") retry c=" << c
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
                // SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
                // event添加成功
                sylar::Fiber::YieldToHold();
                // SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
                // 如果fiber 被唤醒回来timer还存在的话，那就取消掉timer
                if (timer)
                {
                    timer->cancel();
                }
                // 说明当前的fiber是通过cancelevent中的trigger触发唤醒的
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

    extern "C"{

        // 对HOOK_FUN里面的所有xx(name)宏 都要初始化 函数指针定义
        #define XX(name) name##_fun name##_f = nullptr;
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
            sylar::IOManager *iom = sylar::IOManager::GetThis();
            iom->addTimer(seconds * 1000, std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) & sylar::IOManager::schedule, iom, fiber, -1));

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
            sylar::IOManager *iom = sylar::IOManager::GetThis();
            iom->addTimer(usec / 1000, std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) & sylar::IOManager::schedule, iom, fiber, -1));
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
            sylar::IOManager *iom = sylar::IOManager::GetThis();
            iom->addTimer(timeout_ms, std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) & sylar::IOManager::schedule, iom, fiber, -1));
            sylar::Fiber::YieldToHold();
            return 0;
        }

        int socket(int domain, int type, int protocol)
        {
            if (!t_hook_enable)
            {
                return socket_f(domain, type, protocol);
            }
            // 如果hook了，那也执行scoket_f，接下来执行额外的操作
            int fd = socket_f(domain, type, protocol);
            if (fd == -1)
            {
                return fd;
            }
            // 得到fd，如果fd不在m_datas中，那就强制添加到m_datas中
            sylar::FdMgr::GetInstance()->get(fd, true);
            return fd;
        }

        int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
        {
            if (!sylar::t_hook_enable)
            {
                return connect_f(sockfd, addr, addrlen);
            }
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
            // 如果ctx不存在或者ctx已经关闭了
            if (!ctx || ctx->isClose())
            {
                errno = EBADE;
                return -1;
            }
            // 如果ctx 不是socket的话
            if (!ctx->isSocket())
            {
                return connect_f(sockfd, addr, addrlen);
            }
            // 如果是用户已经设定了 O_NONBLOCK
            if (ctx->getUserNonblock())
            {
                return connect_f(sockfd, addr, addrlen);
            }

            int n = connect_f(sockfd, addr, addrlen);
            // success
            if (n == 0)
            {
                return 0;
            }
            else if (n != -1 || errno != EINPROGRESS)
            {
                return n;
            }
            // 加下来就是 n == -1 && errno == EINPROGRESS 的情况
            sylar::IOManager *iom = sylar::IOManager::GetThis();
            sylar::Timer::ptr timer;
            // 设置条件定时器
            std::shared_ptr<timer_info> tinfo(new timer_info);
            std::weak_ptr<timer_info> winfo(tinfo);

            // 有超时时间
            if (timeout_ms != (uint64_t)-1)
            {
                timer = iom->addConditionTimer(
                    timeout_ms, [winfo, sockfd, iom]()
                    {
                        // 创建一个share_ptr指针t，共享winfo指向内存空间的权利
                        auto t = winfo.lock();
                        // 如果t不存在亦或者t的cancelled == true (意味着被取消了)
                        if (!t || t->cancelled)
                        {
                            return;
                        }
                        // 如果t存在，那么设置条件超时
                        t->cancelled = ETIMEDOUT;
                        iom->cancelEvent(sockfd, sylar::IOManager::WRITE);},
                    winfo);
            }
            int rt = iom->addEvent(sockfd, sylar::IOManager::WRITE);
            if (rt == 0)
            {
                sylar::Fiber::YieldToHold();
                // 如果被唤醒的话 --
                // timer还存在， 那么就取消定时器
                if (timer)
                {
                    timer->cancel();
                }
                // 如果tinfo条件被取消了
                if (tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }
            }
            else
            {
                // 如果添加时间失败了
                // 还有定时器就取消定时器
                if (timer)
                {
                    timer->cancel();
                }
                SYLAR_LOG_ERROR(g_logger) << "connnet addEvent(" << sockfd << ", WRITE) error";
            }
            int error = 0;
            socklen_t len = sizeof(int);
            // 看有没有error
            /*
                如果未发生错误， 则 getsockopt 返回零。 否则，将返回SOCKET_ERROR值
            */
            if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len))
            {
                return -1;
            }
            // 如果没有错误 error = 0(false) !error = true
            if (!error)
            {
                return 0;
            }
            else
            {
                // 有错误
                errno = error;
                return -1;
            }
        }

        int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
        {
            return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timout);
        }

        int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
        {
            if (!t_hook_enable)
            {
                return accept_f(sockfd, addr, addrlen);
            }

            int fd = do_io(sockfd, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
            if (fd >= 0)
            {
                sylar::FdMgr::GetInstance()->get(fd, true);
            }
            return fd;
        }

        ssize_t read(int fd, void *buf, size_t count)
        {
            return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
        }

        ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
        {
            return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
        }

        ssize_t recv(int sockfd, void *buf, size_t len, int flags)
        {
            return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
        }

        ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                         struct sockaddr *src_addr, socklen_t *addrlen)
        {
            return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
        }

        ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
        {
            return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ, SO_RCVTIMEO, msg, flags);
        }

        ssize_t write(int fd, const void *buf, size_t count)
        {
            return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
        }

        ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
        {
            return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
        }

        ssize_t send(int sockfd, const void *buf, size_t len, int flags)
        {
            return do_io(sockfd, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
        }

        ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                       const struct sockaddr *dest_addr, socklen_t addrlen)
        {
            return do_io(sockfd, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
        }

        ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
        {
            return do_io(sockfd, sendmsg_f, "sendmsg", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
        }

        int close(int fd)
        {
            if (!sylar::t_hook_enable)
            {
                return close_f(fd);
            }

            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
            if (ctx)
            {
                auto iom = sylar::IOManager::GetThis();
                if (iom)
                {
                    // 取消与这个fd相关的所有事件
                    iom->cancelAll(fd);
                }
                // 删除m_datas中的fd
                sylar::FdMgr::GetInstance()->del(fd);
            }
            return close_f(fd);
        }

        int fcntl(int fd, int cmd, ... /* arg */)
        {
            va_list va;
            // 可以获取可变参数(...)参数列表中的第一个参数的地址并由va_list类型返回
            //  va -> ...
            va_start(va, cmd);
            switch (cmd)
            {
            // int类型的 --- cmd类型代表了后续参数的类型
            case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                // FdManager类型的指针调用get返回了fdctx类型的指针
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                // 如果ctx不存在或者ctx存在但是fd已经被关闭了
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return fcntl_f(fd, cmd, arg);
                }

                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock())
                {
                    // 如果是系统设置的nonblock,那就添加上非阻塞
                    arg |= O_NONBLOCK;
                }
                else
                {
                    // 如果不是, 那就删除掉非阻塞
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return arg;
                }
                if (ctx->getUserNonblock())
                {
                    return arg | O_NONBLOCK;
                }
                else
                {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            case F_SETPIPE_SZ:
            {
                // 获取可变参数的当前参数，返回指定类型并将指针指向下一参数
                int arg = va_arg(va, int);
                // 清空va_list 可变参数列表
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            // void类型的 --- cmd类型代表了后续参数的类型
            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
            case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
            // struct (flock)类型的 --- cmd类型代表了后续参数的类型
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            {
                struct flock *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            // struct (f_owner_exlock)类型的 --- cmd类型代表了后续参数的类型
            case F_GETOWN_EX:
            case F_SETOWN_EX:
            {
                struct f_owner_ex *arg = va_arg(va, struct f_owner_ex *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
            default:
                va_end(va);
                return fcntl_f(fd, cmd);
            }
        }

        int ioctl(int fd, unsigned long request, ...)
        {
            va_list va;
            va_start(va, request);
            void *arg = va_arg(va, void *);
            va_end(va);

            if (FIONBIO == request)
            {
                // arg先强转成 int*类型，然后*是取值，随后！！是将值转化为0或者1
                bool user_nonblock = !!*(int *)arg;
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || ctx->isSocket())
                {
                    return ioctl_f(fd, request, arg);
                }
                ctx->setUserNonblock(user_nonblock);
            }
            return ioctl_f(fd, request, arg);
        }

        int getsockopt(int sockfd, int level, int optname,
                       void *optval, socklen_t *optlen)
        {
            return getsockopt_f(sockfd, level, optname, optval, optlen);
        }

        int setsockopt(int sockfd, int level, int optname,
                       const void *optval, socklen_t optlen)
        {
            if (!sylar::t_hook_enable)
            {
                setsockopt_f(sockfd, level, optname, optval, optlen);
            }
            if (level == SOL_SOCKET)
            {
                // 判断是否有超时时间--根据超时时间类型进行匹配
                if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
                {
                    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
                    if (ctx)
                    {
                        const timeval *tv = (const timeval *)optval;
                        ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
                    }
                }
            }
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
    }

}
