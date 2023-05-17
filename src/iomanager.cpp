#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
#if 1
    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }
    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
    void IOManager::FdContext::triggerEvent(IOManager::Event event)
    {
        SYLAR_ASSERT(events & event);
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }

        ctx.scheduler = nullptr;
        return;
    }

    // 子类的构造函数需要先 实现父类的构造函数
    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name) // 继承了父类的构造函数
    {
        // On sucesses, return a file descriptor(fd);On  error, -1 is returned
        m_epfd = epoll_create(5000); // 创建一个文件句柄, m_epfd = 3
        SYLAR_ASSERT(m_epfd > 0);

        // 生成管道 On success, zero is returned.  On error, -1 is returned
        // 管道是 0作为读，1作为写
        /*
        内核的文件描述符 前三个文件描述符在内核启动的时候就被占用了。
        第0个是stdin，第1个是stdout，第2个是strerr。
        接下来的每当有一个文件产生，就会占就最近的空闲文件描述符。
        */
        int rt = pipe(m_tickleFds); // m_tickleFds[0] = 4, m_tickleFds[0] = 5
        if (rt == -1)
        {
            perror("pipe");
        }
        SYLAR_ASSERT(!rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event)); // 清空所分配的内存
        event.events = EPOLLIN | EPOLLET;       // 为 读事件 和边缘触发模式
        // 设置读端 --- 系统还空闲的文件描述符
        event.data.fd = m_tickleFds[0];

        /*
         fcntl()的 F_SETFL 命令来修改打开文件的某些状态标志。允许更改的标志有
            O_APPEND、O_NONBLOCK、O_NOATIME、O_ASYNC 和 O_DIRECT。
        O_NONBLOCK和O_NDELAY所产生的结果都是使I/O变成非阻塞模式(non-blocking)，
        在读取不到数据或是写入缓冲区已满会马上return，而不会阻塞等待。
        */
        // file control，提供了对文件描述符的各种操作 --- 控制
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        SYLAR_ASSERT(!rt);
        // int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
        //  fd 所关联的事件event  --- 关联事件是 读事件，然后是EPOLLIN | EPOLLET
        // 在文件描述符epfd所引用的epoll实例上注册目标文件描述符fd，并将事件事件与内部文件链接到fd
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        SYLAR_ASSERT(!rt);
        // 设置 m_fdContexts大小基础为32个
        contextResize(32);

        // Scheduler 中的start()
        start();
    }
    IOManager::~IOManager()
    {
        SYLAR_LOG_INFO(g_logger) << "~IOManager start stop";
        stop();
        SYLAR_LOG_INFO(g_logger) << "~IOManager end stop";
        // 关闭fd句柄
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);
        // 释放内存
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            // 为空就要初始化一个fdContext
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    // 1 success, 0 retry, -1 error
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            // 解 读锁
            lock.unlock();
        }
        else
        {
            // 解 读锁
            lock.unlock();
            // 上 写锁
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->events & event)
        {
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                      << "event=" << event
                                      << "fd_ctx.event=" << fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));
        }

        // 如果是NONE，那就是EPOLL_CTL_ADD(添加事件)，如果不是那就是EPOLL_CTL_MOD(修改事件)
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;
        epevent.data.fd = fd;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return -1;
        }

        // 添加事件
        ++m_pendingEventCount;
        // 当前事件的  fd_ctx->events(原本有的事件) | event（外部传入的事件），看是READ还是WRITE
        fd_ctx->events = (Event)(fd_ctx->events | event);
        // 返回 READ事件 或者 WRITE事件
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
        // 设置为当前调度器
        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            // 如果是fun，那么就执行fun
            SYLAR_LOG_INFO(g_logger) << "cb";
            event_ctx.cb.swap(cb);
            // sylar::Scheduler::schedule(event_ctx.cb);
        }
        else
        {
            // 如果不是fun，那么就执行fiber
            event_ctx.fiber = Fiber::GetThis();
            SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
        }
        return 0;
    }

    // 删除事件
    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        // ~ 操作是 非
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }
    // 取消事件， 将事件需要一定条件要出发，并将该事件强制触发掉
    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event))
        {
            return false;
        }

        // ~ 操作是 非
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        // 触发事件后，取消掉事件
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }
    // 取消一个句柄下的全部事件
    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }

        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        // ~ 操作是 非
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        SYLAR_ASSERT(fd_ctx->events == 0);

        return true;
    }
    // 获取当前的 IOManager
    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    void IOManager::tickle()
    {
        // 是否还有空闲的线程，如果有则写入数据，并让空闲线程执行
        if (!hasIdleThreads())
        {
            return;
        }
        // int __fd, const void *__buf, size_t __n;  return 实际写入数据的长度
        int rt = write(m_tickleFds[1], "T", 1);
        // SYLAR_LOG_INFO(g_logger) << "io tickle";
        SYLAR_ASSERT(rt == 1);
    }

    bool IOManager::stopping()
    {
        // SYLAR_LOG_INFO(g_logger) << "m_pendingEventCount=" << m_pendingEventCount;
        uint64_t timeout = 0;
        return stopping(timeout);
    }
    bool IOManager::stopping(uint64_t &timeout)
    {
        // 查看是否还有定时器任务未完成
        timeout = getNextTimer();
        // 定时器没有了任务，并且当前event也没有了
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    // 空闲时刻执行的任务
    void IOManager::idle()
    {
        SYLAR_LOG_INFO(g_logger) << " in idle";
        // 创建一个events类型的数组
        const uint64_t MAX_EVNETS = 64;
        epoll_event *events = new epoll_event[MAX_EVNETS]();
        // 让智能指针指向该数组，用于内存释放
        std::shared_ptr<epoll_event> shared_event(events, [](epoll_event *ptr)
                                                  { delete[] ptr; });
        while (true)
        {//基于特征空间的递归框架将改善群智能算法在基因选择上的性能
            // 如果结束
            uint64_t next_timeout = 0;
            if (stopping(next_timeout))
            {
                SYLAR_LOG_INFO(g_logger) << "name=" << Scheduler::getName()
                                         << "idle stopping exit";
                break;
            }
            int rt = 0;
            // 如果没有结束
            do
            {

                // 定时器，最多等待5000ms
                static const int MAX_TIMEOUT = 3000;
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }

                // 等待
                // int epoll_wait(int __epfd, epoll_event *__events, int __maxevents, int __timeout)
                rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
                SYLAR_LOG_INFO(g_logger) << "rt = " << rt;
                if (rt < 0 && errno == EINTR)//rt 小于0并且errno==EINTR是异常中断
                {
                    perror("epoll_wait");
                }
                else
                {
                    break;
                }
            } while (true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            // SYLAR_LOG_INFO(g_logger) << " cbs size "<< cbs.size(); 
            if (!cbs.empty())
            {
                SYLAR_LOG_INFO(g_logger) << "on timer cbs.size=" << cbs.size();
                // 将所有任务都存入到调度器中
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            for (int i = 0; i < rt; ++i)
            {
                epoll_event &event = events[i];
                SYLAR_LOG_INFO(g_logger) << "event.fd = " << event.data.fd;
                // 如果是读端
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy[256];
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
                        ;
                    continue;
                }

                // void* 类型的指针转化为其他类型 需要强制转化
                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }

                int real_events = NONE;
                // 如果event.events 是IN，那么就要READ,因为是&操作，不都为1就为0
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                // 如果event.events 是OUT，那么就要WRITE
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }
                // 什么事件都没有
                if ((fd_ctx->events & real_events) == NONE)
                {
                    continue;
                }
                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ","
                                              << op << "," << fd_ctx->fd << "," << event.events << "):"
                                              << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                if (real_events & READ)
                {
                    // 触发读事件
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    // 触发写事件
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_cur = cur.get();
            // 释放当前Fiber
            cur.reset();
            SYLAR_LOG_INFO(g_logger) << "io idle end";

            raw_cur->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }

#else

#endif

} // namespace sylar
