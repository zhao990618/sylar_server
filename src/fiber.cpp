#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar
{

    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    // 局部指针 -- 指向当前线程
    /* thread_local 修饰的对象在 前线程创建时生成，在线程销毁是灭亡；
        thread_local 一般用于需要保证线程安全的函数中，
        如果类的成员函数中定义了 thread_local变量，则对于
        同一个线程 内的该类的多个对象都会共享一个变量实例 --
    */
    // 该线程中 正在运行的协程 -- 调用的就是 private修饰的构造函数
    static thread_local Fiber *t_fiber = nullptr;
    // 该线程中 的主协程  --
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    static ConfigVar<uint32_t>::ptr g_fiber_fiber_stack =
        Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

    class MallocStackAllocator
    {
    public:
        // 开辟空间
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }
        // 释放空间
        static void Dealloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    typedef MallocStackAllocator StackAllocator;

    // private 类型的构造函数， 被当作main协程
    Fiber::Fiber()
    {
        // 正在执行
        m_state = Fiber::EXEC;
        // 将当前线程设置为t_fiber
        SetThis(this);

        // 将当前线程 所包含的运行时的cpu状态，寄存器状态，栈内存状态传递给当前的协程
        if (getcontext(&m_ctx))
        {
            // When  successful,  getcontext() returns 0
            SYLAR_ASSERT2(false, "getcontext");
        }
        // 协程数量+1
        s_fiber_count++;

        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main " << m_id;
    }
    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
        : m_id(++s_fiber_id), m_cb(cb)
    {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_fiber_stack->getValue();
        m_stack = StackAllocator::Alloc(m_stacksize);
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }
        // 无上下文。当协程执行完毕后 接着执行uc_link，若uc_link == NULL,则退出线程
        m_ctx.uc_link = nullptr;
        // m_ctx.uc_link = &t_threadFiber->m_ctx;
        // 让子协程链接主协程，子协程结束后立刻跳转回主协程
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        if (!use_caller)
        {
            makecontext(&m_ctx, &Fiber::MainFunc, 0);
        }
        else
        {
            makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        }

        SYLAR_LOG_INFO(g_logger) << "Fiber::Fiber id=" << m_id;
    }
    Fiber::~Fiber()
    {
        // 协程数量-1
        --s_fiber_count;
        if (m_stack)
        {
            // 如果有栈的空间
            SYLAR_ASSERT(m_state == INIT || m_state == TERM || m_state == EXCEPT);
            StackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else
        {
            // 如果没有栈空间，那么就是 main协程
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state == EXEC);
            Fiber *cur = t_fiber;
            if (cur == this)
            {
                Fiber::SetThis(nullptr);
            }
        }
        SYLAR_LOG_INFO(g_logger) << "Fiber::~Fiber id=" << m_id
                                  << " total Fiber=" << s_fiber_count;
    }

    // 重置携程函数，并且重置状态
    void Fiber::reset(std::function<void()> cb)
    {
        // 协程结束任务后，不释放内存，将该内存用于新的协程
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_state == INIT || m_state == TERM || m_state == EXCEPT);
        m_cb = cb;
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);

        m_state = INIT;
    }

    // 等于swapIn();
    void Fiber::call()
    {
        SetThis(this); // 子协程调用
        // SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
    // 等于swapOut();
    void Fiber::back()
    {
        SetThis(t_threadFiber.get());

        // swapcontext(old,new)
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    // 当前子协程开始执行，获取执行权，切入执行队列
    void Fiber::swapIn()
    {
        // 将目标协程唤醒到当前执行状态
        // 即目标协程和当前运行的协程互相切换，当前运行的协程切换到后台，目标协程开始执行
        // 一般来说 目标协程是 子协程
        SetThis(this); // 子协程调用
        SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        // SYLAR_LOG_INFO(g_logger) << "before swapcontext";
        // 交换主协程和 目标协程之间的关系 -- swapcontext(old,new)
        if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
        // SYLAR_LOG_INFO(g_logger) << "after swapcontext";
    }
    // 当前子协程结束执行，让出执行权，切换回mian fiber
    void Fiber::swapOut()
    {
        // 讲当前协程切换到后台，将main协程切换出来
        SetThis(Scheduler::GetMainFiber());

        // swapcontext(old,new)
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
    // 获得当前协程ID
    uint64_t Fiber::GetFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }
    // 设置当前协程
    void Fiber::SetThis(Fiber *f)
    {
        // 将当前协程 设置对应的参数
        t_fiber = f;
    }
    // 返回当前协程
    Fiber::ptr Fiber::GetThis()
    {
        // 获取当前正在执行的协程
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        // 如果当前协程不存在，那么就说明当前没有协程---则创建一个主协程
        Fiber::ptr main_fiber(new Fiber); // --> SetThis(this) --> t_fiber = this
        // .get 是得到当前指针所指向的内容 -- 当前协程就等与 主协程
        SYLAR_ASSERT(t_fiber == main_fiber.get());
        // 重新设置主协程
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }
    // 当前协程切换到后台，并且设置为ready状态, 并且切换到主协程上
    void Fiber::YieldToReady()
    {
        // 操作当前正在执行的协程 设置为 raeady状态，并且切换回主协程上去
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = READY;
        // 切换出去-- 变成主协程
        cur->swapOut();
    }
    // 当前协程切换到后台，并且设置为hold状态，并且切换到主协程上
    void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = HOLD;
        // 切换出去-- 变成主协程
        cur->swapOut();
    }
    // 总协程数量
    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

    void Fiber::MainFunc()
    {
        // 得到当前协程  ---- cur 获得了当前fiber的控制权副本，引用计数+1
        Fiber::ptr cur = GetThis(); //
        SYLAR_ASSERT(cur);
        try
        {
            // SYLAR_LOG_INFO(g_logger) << "2";
            cur->m_cb();
            // SYLAR_LOG_INFO(g_logger) << "3";
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (const std::exception &e)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except" << e.what()
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }

        /*
        cur是一个指针指向了fiber内存空间，fiber结束后需要释放掉该指针，但是该fiber
        需要返回 主fiber，则无法直接释放（直接释放，则找不到该fiber，无法返回main fiber）。
        所以需要一个新的指针指向该内存（fiber），然后释放掉fiber内存
        */

        auto raw_ptr = cur.get();
        cur.reset(); // cur 指向空
        // 后续raw_ptr析构会释放掉当前fiber
        raw_ptr->swapOut();
        // raw_ptr->call();

        SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
    }

    void Fiber::CallerMainFunc()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->back();
        SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
    }

} // namespace sylar
