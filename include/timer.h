#pragma once

#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <thread.h>
#include <vector>
#include <set>

namespace sylar
{
    class TimerManager;

    class Timer : public std::enable_shared_from_this<Timer>
    {
        // 通过TImeManager 来创建对象
        friend class TimerManager;
        public:
            typedef std::shared_ptr<Timer> ptr;
            bool cancel();
            bool refresh();
            bool reset(uint64_t ms, bool from_now);
        private:
            Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
            Timer(uint64_t next);

        private:
            // 是否为循环定时器，若为true，则循环执行函数
            bool m_recurring = false;
            // 间隔时间
            uint64_t m_ms = 0;
            // 下一个执行周期的时间;精确的执行事件
            uint64_t m_next = 0;
            TimerManager* m_manager = nullptr; // Timer是属于哪一个TimerManager
            std::function<void()> m_cb;

        private:
            struct Comparator
            {
                // 即无比较函数,那么默认就会使用 地址 来进行比较
                // bool operator()(const Timer::ptr& lhs, const Timer::ptr rhs) const {};
                bool operator()(const Timer::ptr& lhs, const Timer::ptr rhs) const;
            };
    };

    class TimerManager
    {
        friend class Timer;
        public:
            typedef RWMutex RWMutexType;

            TimerManager();
            virtual ~TimerManager();

            // 时间定时器
            Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                               ,bool recurring = false);
            // 条件定时器 --- 满足条件才定时器才会触发
            Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                                        ,std::weak_ptr<void> weak_ptr
                                        ,bool recurring = false);
            // 得到下一个定时器的执行时间
            uint64_t getNextTimer();
            // 把已经超时了的定时器 所需要执行的回调函数收集起来，并且执行
            void listExpiredCb(std::vector<std::function<void()> >& cbs);
            bool hasTimer();
        protected:
            virtual void onTimerInsertedAtFront() = 0;
            void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
            // 当前是否存在定时器

        private:
            // 检测是否出现修改系统时间的现象，出现了之后使得系统改变措施
            bool detectClockRollover(uint64_t now_ms);
        private:
            RWMutexType m_mutex;
            // set是有序的 --- hash表来存储ptr
            std::set<Timer::ptr, Timer::Comparator> m_timers;
            bool m_tickled = false;
            uint64_t m_previouseTime;
    };
}

#endif