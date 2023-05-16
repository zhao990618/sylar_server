#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include "functional"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static thread_local Scheduler *t_scheduler = nullptr;
    // 主协程
    static thread_local Fiber *t_scheduler_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
        : m_name(name)
    {
        SYLAR_ASSERT(threads > 0);
        // use_caller的线程 m_rootThread != -1
        if (use_caller)
        {
            // 设置当前线程的名称  --- 一个进程 默认配备了一个线程
            sylar::Thread::SetName(m_name);
            // 创建了一个线程后；线程数量-1
            --threads;
            // 得到当前协程，若没有协程则创建一个主协程（主协程没有func）
            sylar::Fiber::GetThis();
            // GetThis()返回的是scheduler类型对象，则如果有scheduler，那么就会报错，因为不能再创建scheduler了
            SYLAR_ASSERT(GetThis() == nullptr);
            // 主协程调度器
            t_scheduler = this;
            /*
            调度器中的主协程
            auto newCallable = bind(callable,arg_list) -- arg_list 是callable的参数
             如果是类内函数，函数的参数中包含一个隐藏的this指针。需要传入类对象，保证这个函数属于哪一个对象
            当前这个run函数属于this这个对象
            */
            // 主协程 是用call 和 back，其他协程都用swapOut，swapIn  -- 用于执行run函数
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));

            // 设置当前协程调度器中的 执行协程
            t_scheduler_fiber = m_rootFiber.get();
            // 得到当前线程ID
            m_rootThreadId = sylar::GetThreadId();
            // 将当前线程ID添加到线程IDs数组中
            m_threadIds.push_back(m_rootThreadId);
        }
        else
        {
            m_rootThreadId = -1;
        }
        // 获得当前 剩余的 线程数量
        m_threadCount = threads;
    }
    Scheduler::~Scheduler()
    {
        SYLAR_ASSERT(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }
    Fiber *Scheduler::GetMainFiber()
    {
        return t_scheduler_fiber;
    }

    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        // 如果m_stopping == false, 则调度器已经启动,则不需要重新启动
        if (!m_stopping)
        {
            return;
        }
        // m_stopping初始化为true，无调度器时为true，开启调度器后变为false
        m_stopping = false;
        // 调度器启动前，线程池为空
        SYLAR_ASSERT(m_threads.empty());
        // 为线程池分配内存大小为m_threadCount * Thread::ptr大小的空间,vector::resize()
        m_threads.resize(m_threadCount);
        // 分配线程
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // ptr.reset() -- 为每一个线程添加名字，名字包含了线程池名字
            // 并且初始化了线程m_id，m_cb，m_name
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            // 添加线程ID到线程IDs数组
            m_threadIds.push_back(m_threads[i]->getId());
        }

        // 解锁
        lock.unlock();

    }

    void Scheduler::stop()
    {
        // SYLAR_LOG_INFO(g_logger) << "stop start";
        m_autostop = true;
        if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        {
            // use_caller == true and 只有一个协程的情况下
            SYLAR_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if (stopping())
            {
                return;
            }
        }

        // bool exit_on_this_fiber = false;
        // use_caller == ture --> m_rootThreadId != -1 -- 需要在创建scheduler那个协程里去执行stopping()
        // use_caller == false --> m_rootThreadId == -1 -- 可以在任意非自己的协程执行stopping()
        if (m_rootThreadId != -1)
        {
            // 若不等于 -1， 说明是添加到调度中的，GetThis() == this
            SYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            SYLAR_ASSERT(GetThis() != this);
        }
        // 停止，就要m_stopping == true
        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // SYLAR_LOG_INFO(g_logger) << "stop log";
            // 类似于发出信号，让挂起的线程被唤醒，其次终结线程
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {

            if (!stopping())
            {
                // SYLAR_LOG_INFO(g_logger) << "call";
                // 让当前主协程让出控制权，m_rootFiber协程介入获得控制权
                m_rootFiber->call();
                // SYLAR_LOG_INFO(g_logger) << "stop call";
            }
        }
        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        // 让主线程停滞，等待子线程结束， 回收子线程的资源
        for (auto &i : thrs)
        {
            i->join();
        }
        // SYLAR_LOG_INFO(g_logger) << "stop end";
    }

    void Scheduler::setThis()
    {
        // 主调度器
        t_scheduler = this;
    }

    // 线程调度的主函数run
    void Scheduler::run()
    {
        // SYLAR_LOG_INFO(g_logger) << "run";
        // 设置是否要hook  --- ture
        set_hook_enable(true);
        // 将当前线程的 schedule设置成本身
        setThis();
        // 如果当前线程的ID != m_rootThreadId  --- 则需要切换成当前线程。
        // 产生的子线程会复制 父线程的所有内容，所以m_rootThread 是父线程的ID，
        // 切换到子线程后，就需要将子线程内的m_rootThread换成子线程自己的ID
        if (sylar::GetThreadId() != m_rootThreadId)
        {
            SYLAR_LOG_INFO(g_logger) << "m_rootThreadId=" << m_rootThreadId << "; GetThreadId()=" << GetThreadId();
            // 指向当前线程 正在执行的协程 -- 如果没有协程那么就创建一个主协程
            t_scheduler_fiber = Fiber::GetThis().get();
        }
        // 当所有调度任务全部完成时，实现idle协程
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        // 回调函数--协程
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true)
        {
            // 在执行前，将所有内容置位Null
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                /*
                    从消息队列中取出 一个 必须要执行的消息
                */
                MutexType::Lock lock(m_mutex);
                // 等待执行的协程队列 的开始
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    // 即当前的协程任务 不适合本线程执行，那么就需要通知其他线程去抢占资源执行run，来执行该实现的fiber
                    // 如果当前的线程存在，并且当前线程不等于当前执行run函数的线程，那么就可以跳过
                    if (it->thread != -1 && it->thread != sylar::GetThreadId())
                    {
                        ++it;
                        // 需要通知 其他线程去实现
                        tickle_me = true;
                        continue;
                    }
                    // 找到了可以执行的任务 --
                    SYLAR_ASSERT(it->fiber || it->cb);
                    // 如果it遇到的协程处于EXEC状态（执行状态），则跳过
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                    {
                        ++it;
                        continue;
                    }
                    // 处理任务
                    ft = *it;
                    m_fibers.erase(it++);
                    // m_fibers.erase(it);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
                tickle_me |= it != m_fibers.end();
            }
            // 通知其他线程
            if (tickle_me)
            {
                // SYLAR_LOG_INFO(g_logger) << sylar::GetThreadId();
                tickle();
            }

            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
            {
                // 协程切入
                ft.fiber->swapIn();
                --m_activeThreadCount;
                // 说明 当前fiber通过YieldToReady让出的执行资源，则让该fiber继续回调度器中等待
                if (ft.fiber->getState() == Fiber::READY)
                {
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    ft.fiber->m_state = Fiber::HOLD;
                }
                // 释放掉ft
                ft.reset();
            }
            else if (ft.cb)
            {
                if (cb_fiber)
                {
                    /* Fiber 自带的reset()，
                       可以让fiber空间不消亡，自动执行新的任务。
                       省去了消亡和创建新的fiber对象的消耗
                    */
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    // 如果 cb_fiber 是一个空指针，那就新建一个fiber
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                // 释放掉ft
                ft.reset();
                // 协程切入执行 （t_scheduler_fiber 所指向的m_rootFiber 与 cb_fiber进行切换）
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if (cb_fiber->getState() == Fiber::READY)
                {
                    // 被fiber通过YieldToReady让出的执行资源，则让该fiber继续回调度器中等待
                    schedule(cb_fiber);
                    // 将cb_fiber放入到调度器后释放掉
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
                {
                    // 当前func 有问题，则剔除掉当前任务
                    cb_fiber->reset(nullptr);
                }
                else
                {
                    // 等待执行状态
                    cb_fiber->m_state = Fiber::HOLD;
                    // ? 为何要释放掉cb_fiber
                    cb_fiber.reset();
                }
            }
            else
            {
                if (is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }
                // 没有fiber任务或者func任务，执行idle
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }
                ++m_idleThreadCount;
                // 协程切入执行 （t_scheduler_fiber 所指向的m_rootFiber 与 idle_fiber进行切换） 
                idle_fiber->swapIn();
                // SYLAR_LOG_INFO(g_logger) << idle_fiber->getState();
                --m_idleThreadCount;
                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            } 
            // SYLAR_LOG_INFO(g_logger) << "while ..";   
        }
        SYLAR_LOG_INFO(g_logger) << "run done";
    }

    void Scheduler::tickle()
    {
        SYLAR_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        // SYLAR_LOG_INFO(g_logger) << "m_autostop = " << m_autostop;
        // SYLAR_LOG_INFO(g_logger) << "m_stopping = " << m_stopping;
        // SYLAR_LOG_INFO(g_logger) << "m_fibers.empty() = " << m_fibers.empty();
        // SYLAR_LOG_INFO(g_logger) << "m_activeThreadCount = " << m_activeThreadCount;
        // SYLAR_LOG_INFO(g_logger) << "stopping()! ";
        return m_autostop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        SYLAR_LOG_INFO(g_logger) << "idle";
        while (!stopping())
        {
            // SYLAR_LOG_INFO(g_logger) << "idle YieldToHold";
            // fiber进来执行上面的idle输出，其他情况下进来就会不断的在while中循环，
            // 直到条件满足，才会退出循环
            sylar::Fiber::YieldToHold();
        }
    }

} // namespace sylar