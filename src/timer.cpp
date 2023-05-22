#include "timer.h"
#include "util.h"
#include "log.h"

#if 1
namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
    {
        m_next = sylar::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next)
        : m_next(next)
    {
    }

    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb)
        {
            m_cb = nullptr;
            // 用智能指针返回 该timer
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }
    // 某些定时器设置时间为5s（实际可能是10:00:05），若10:00:03秒接收到了数据，那么需要重新刷新时间变回10:00:08
    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }

        auto it = m_manager->m_timers.find(shared_from_this());
        // 如果这个定时器已经不在 m_manager中了，那么返回false
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        // set 不可以修改key值，所以智能删除(erase)，重新添加新的时间10:00:08（set要根据新的key来排序）
        m_manager->m_timers.erase(it);
        m_next = sylar::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }
    bool Timer::reset(uint64_t ms, bool from_now)
    {
        // from_now 表示是否需要立马强制改变时间；
        if (ms == m_ms && !from_now)
        {
            return true;
        }

        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        // 找到该定时器
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())// 找不到就退出
        {
            return false;
        }
        // set 不可以修改key值，所以智能删除(erase)，重新添加新的
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        // 是否要立即修改
        if (from_now)
        {
            start = sylar::GetCurrentMS();
        }
        else
        {
            start = m_next - m_ms;
        }
        // 更新ms，从外部传进来的ms
        m_ms = ms;
        // 基于更新后的ms，来更新下一次执行的时间 m_next
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr rhs) const
    {
        // lhs 和 rhs 不存在则返回false， 如果lhs不存在那就是false
        if (!lhs && !rhs) // lhs == true ---> !lhs == false,直接退出
        {
            return false;
        }
        // lhs不存在出来后，rhs， 判断rhs是否存在
        if (!lhs)
        {
            return true;
        }
        // lhs存在出来后， 判断rhs是否存在
        if (!rhs)
        {
            return false;
        }
        // 判断 大小
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (rhs->m_next < lhs->m_next)
        {
            return false;
        }
        // 如果完全一样
        return lhs.get() < rhs.get();
    }

    TimerManager::TimerManager()
    {
        m_previouseTime = sylar::GetCurrentMS();
        // std::cout << "TimerManager create" << std::endl;
    }

    TimerManager::~TimerManager()
    {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        // /*
        // std::pair<iterator, bool>
        // insert(const value_type& __x)
        // */
        // // 因为set 是一个有序的列表，而insert插入则会返回当前插入位置的索引。
        // auto it = m_times.insert(timer).first;
        // // 因此，如果insert返回值非常靠前的时候，反应出插入的定时器的事件非常短，是一个马上执行的定时器
        // bool at_front = (it == m_times.begin());
        // lock.unlock();
        // // 这种定时器是需要 一个协程去取消eopll_wait()
        // if (at_front)
        // {
        //     onTimerInsertedAtFront();
        // }

        addTimer(timer, lock);

        // 可以凭借这个timer，来实现业务上的 取消操作
        return timer;
    }

    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock)
    {
        auto it = m_timers.insert(val).first;
        // 因此，如果insert返回值非常靠前的时候，反应出插入的定时器的事件非常短，是一个马上执行的定时器
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front)
        {
            m_tickled = true;
        }
        lock.unlock();
        // 这种定时器是需要 一个协程去取消eopll_wait()
        if (at_front)
        {
            onTimerInsertedAtFront();
        }
        // 可以凭借这个timer，来实现业务上的 取消操作
    }

    // 查看当前系统时间是否被修改
    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        // 当前时间小于 之前的时间 并且 当前时间和之前的时间差距 大于 1小时
        if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            // 系统认为时间被修改过
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }

    // weak_ptr 平不会让引用计数器＋1，并且可以直到所指向的内存空间是否被释放掉
    static void Ontimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        // 返回当前weak_cond指针的智能指针，如果有指向则返回智能指针，无指向则是空
        std::shared_ptr<void> tmp = weak_cond.lock();
        // 如果智能指针指向的内存空间存在则执行cb(),如果内存已经释放则跳过
        if (tmp)
        {
            cb();
        }
    }

    // 需要处理唤醒的机制
    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_ptr, bool recurring)
    {
        // bind(func(), arg1, arg2); --- 将func()和其所包含的两个参数一起
        return addTimer(ms, std::bind(&Ontimer, weak_ptr, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        // 如果是为 空，则说明没有定时器
        if (m_timers.empty())
        {
            // 0取反，是一个最大的数
            return ~0ull;
        }

        // 获得m_timers中的第一个定时器，next是引用类型，引用的值是m_times.begin()的值
        const Timer::ptr &next = *m_timers.begin();
        // 得到当前的时间
        uint64_t now_ms = sylar::GetCurrentMS();
        // 当前时间超过了 定时器序列中的第一个定时器下一次所执行的时间
        if (now_ms >= next->m_next)
        {
            return 0;
        }
        else
        {   
            // 返回还需要等待的时间
            return next->m_next - now_ms;
        }
    }

    bool TimerManager::hasTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = sylar::GetCurrentMS();
        // 存放已经超时了的timer
        std::vector<Timer::ptr> expired;

        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty())
            {
                return; 
            }
        }

        RWMutexType::WriteLock lock2(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
        // 查看是否被修改时间
        bool rollover = detectClockRollover(now_ms);
        // 若没有被修改时间，并且定时器内的所有定时器 都没有超时
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        {
            // 直接退出
            return;
        }
        Timer::ptr now_timer(new Timer(now_ms));
        // lower_bound 是在制定 区域内查找 不小于目标的数据
        // 即，有可能找到的是 大于等于 目标值的数据---所以是下界
        // 如果系统时间发生了变化，那么就直接结束，将整个timer清理
        // 如果 修改过时间并且存在超时，那么就所有定时器都需要 被清理；如果 没有修改过时间但是存在超时现象，超时的定时器需要被清理
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while (it != m_timers.end() && (*it)->m_next == now_ms)
        {
            ++it;
        }
        // 向expired中第begin（）个位置插入m_timer.begin(),it个timer
        expired.insert(expired.begin(), m_timers.begin(), it);
        // 删除掉超时的定时器
        m_timers.erase(m_timers.begin(), it);
        // 将超时的定时器中的 cd函数 都放入到cbs中
        cbs.reserve(expired.size());
        // 更新cbs，如果有定时器是循环定时器(m_recurring == true)，需要重新定时，并将该定时器放回定时器数组中
        for (auto &timer : expired)
        {
            cbs.push_back(timer->m_cb);
            // 如果timer 是循环定时器；循环定时器是一直更新的
            if (timer->m_recurring)
            {
                // 重置时间 = 当前时间 + 间隔时间
                timer->m_next = now_ms + timer->m_ms;
                // 将定时器重新放回 定时器数组中
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb = nullptr;
            }
        }
    }

}
#else

namespace sylar
{
    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr rhs) const
    {
        // lhs 和 rhs 不存在则返回false， 如果lhs不存在那就是false
        if (!lhs && !rhs) // lhs == true ---> !lhs == false,直接退出
        {
            return false;
        }
        // lhs不存在出来后，rhs， 判断rhs是否存在
        if (!lhs)
        {
            return true;
        }
        // lhs存在出来后， 判断rhs是否存在
        if (!rhs)
        {
            return false;
        }
        // 判断 大小
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (rhs->m_next < lhs->m_next)
        {
            return false;
        }
        // 如果完全一样
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb,
                 bool recurring, TimerManager *manager)
        : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager)
    {
        m_next = sylar::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next)
        : m_next(next)
    {
    }

    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb)
        {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        m_manager->m_timers.erase(it);
        m_next = sylar::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if (ms == m_ms && !from_now)
        {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if (from_now)
        {
            start = sylar::GetCurrentMS();
        }
        else
        {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    TimerManager::TimerManager()
    {
        m_previouseTime = sylar::GetCurrentMS();
    }

    TimerManager::~TimerManager()
    {
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }

    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp)
        {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty())
        {
            return ~0ull;
        }

        const Timer::ptr &next = *m_timers.begin();
        uint64_t now_ms = sylar::GetCurrentMS();
        if (now_ms >= next->m_next)
        {
            return 0;
        }
        else
        {
            return next->m_next - now_ms;
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = sylar::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }
        bool rollover = detectClockRollover(now_ms);
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while (it != m_timers.end() && (*it)->m_next == now_ms)
        {
            ++it;
        }
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for (auto &timer : expired)
        {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring)
            {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                timer->m_cb = nullptr;
            }
        }
    }

    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock)
    {
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front)
        {
            m_tickled = true;
        }
        lock.unlock();

        if (at_front)
        {
            onTimerInsertedAtFront();
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }

    bool TimerManager::hasTimer()
    {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

}
#endif