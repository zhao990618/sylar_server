#pragma once
#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include "fiber.h"
#include "thread.h"
#include <vector>
#include <list>

namespace sylar
{
    class Scheduler
    {
    public:
        // 类型定义需要加上 typedef 
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;
        // threads 线层数量， use_caller 讲协程纳入到协程调度器中， name 线程池的名称
        Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        virtual ~Scheduler();

        const std::string &getName() const { return m_name;}
        // 得到主调度器
        static Scheduler* GetThis();
        // 得到主协程
        static Fiber* GetMainFiber();

        void start();
        void stop();

        // 单个任务的调度
        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1)
        {
            bool need_tickle = false;
            {
                // 在局部空间上锁，除了这个空间就会解锁
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }
            if (need_tickle)
            {
                tickle();
            }
        }

        // 多个任务的调度
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end)
                {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
            }
            if (need_tickle)
            {
                tickle();
            }
        }

    protected:
        virtual void tickle();
        virtual bool stopping();
        virtual void idle();
        void setThis();
        void run();
        // 返回闲置的线程数量, 为true则说明还有空闲的线程
        bool hasIdleThreads(){return m_idleThreadCount > 0;}
    private:
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread)
        {
            // 是否为空 
            /*若果为true，则说明当前没有可以执行的任务队列,所有的线程处于内核态或者在wait信号量*/
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            // 如果传入的fc 是fiber 或者func，则可以将该struct传入list（fiber队列）
            if (ft.thread || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }


    private:
        // 用来规定 调度器里面可以调用什么 fiber thread functional
        struct FiberAndThread
        {
            Fiber::ptr fiber;
            std::function<void()> cb;
            // 线程ID --- 用来判断 当前的任务是否需要在线程上实现
            int thread;

            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(f), thread(thr)
            {
            }
            FiberAndThread(Fiber::ptr *f, int thr)
                : thread(thr)
            {
                fiber.swap(*f);
            }
            FiberAndThread(std::function<void()> f, int thr)
                : cb(f), thread(thr)
            {
            }
            FiberAndThread(std::function<void()> *f, int thr)
                : thread(thr)
            {
                cb.swap(*f);
            }
            // stl 初始化对象的时候，必须要一个默认构造函数，否则无法初始化
            FiberAndThread()
                : thread(-1)
            {
            }

            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    private:
        MutexType m_mutex;
        // 线程池
        std::vector<Thread::ptr> m_threads;
        // 等待执行的协程队列
        std::list<FiberAndThread> m_fibers;
        std::string m_name;
        // 可执行的协程
        Fiber::ptr m_rootFiber;

    protected:
        // 保存所有线程的ID，用于选择哪一个线程来执行 协程队列中的任务
        std::vector<int> m_threadIds;
        // 线程数量
        size_t m_threadCount = 0;
        // 活跃线程数量 -- 原子变量
        std::atomic<size_t> m_activeThreadCount = {0};
        // 空闲线程数量
        std::atomic<size_t> m_idleThreadCount = {0};
        // 是否停止
        bool m_stopping = true;
        // 是否主动停止
        bool m_autostop = false;
        // 主线程ID
        int m_rootThreadId = 0;
    };
}

#endif
