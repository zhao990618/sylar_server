#pragma once

#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

// #include <memory>
#include "thread.h"
#include <functional>
#include <ucontext.h>


namespace sylar
{
    class Scheduler;
    class Fiber: public std::enable_shared_from_this<Fiber>
    {
        // 识别不了Scheduler, 需要提前添加声明 class Scheduler
        friend class Scheduler;
        public:
            typedef std::shared_ptr<Fiber> ptr;

            enum State
            {
                INIT, // 初始化
                HOLD, // 保持
                EXEC, // 执行
                TERM, // 终止
                READY,
                EXCEPT    // 错误
            };
        private:
            // 通过静态的成员函数可以调用 private修饰的 构造函数
            Fiber();
        public:
            Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
            ~Fiber();

            // 当前协程运行完毕后，线程给当前协程所分配的内存并不释放，直接将当前内存给新的协程
            // 减少协程的创建内存和释放内存锁消耗的资源
            // 重置携程函数，并且重置状态
            void reset(std::function<void()> cb);
            // 当前协程开始执行，获取执行权
            void swapIn();
            // 当前协程结束执行，让出执行权
            void swapOut();
            // 等于swapIn(); 是用于协程之间的切换，而不是类似于swapIn中主协程和子协程的切换
            void call();
            // 等于 swapOut()
            void back();
            // 得到Id
            uint64_t getId() const {return m_id;}
            State getState() const {return m_state;}
        public:
            // 获得当前协程ID
            static uint64_t GetFiberId();
            // 设置当前协程
            static void SetThis(Fiber* f);
            // 返回当前协程
            static Fiber::ptr GetThis();
            // 协程切换到后台，并且设置为ready状态
            static void YieldToReady();
            // 协程切换到后台，并且设置为hold状态
            static void YieldToHold();
            //总协程数量
            static uint64_t TotalFibers();
            // 主执行函数
            static void MainFunc();
            static void CallerMainFunc();
        private:
            uint64_t m_id = 0;
            uint64_t m_stacksize = 0;
            State m_state = INIT;   // 除了Fiber()无参的构造函数产生的主协程初始化状态为EXEC，其他都为INIT
            ucontext_t m_ctx;
            void* m_stack = nullptr;
            std::function<void()> m_cb;
    };
}

#endif