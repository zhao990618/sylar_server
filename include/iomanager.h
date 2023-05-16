#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar
{
    class IOManager : public Scheduler, public TimerManager
    {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;
        enum Event
        {                // enum 枚举类型需要用 "," 来分割
            NONE = 0x0,  // 无事件
            READ = 0x1,  // 读事件  EPOLLIN
            WRITE = 0x4, // 写事件  EPOLLOUT
        };

    private:
        /**
         * @brief Socket事件上线文类
         */
        struct FdContext
        {
            typedef Mutex MutexType;
            /**
             * @brief 事件上线文类
             */
            struct EventContext
            {
                Scheduler *scheduler = nullptr; // 表示事件要在哪一个scheduler上执行
                Fiber::ptr fiber;               // 事件的协程
                std::function<void()> cb;       // 事件的回调函数
            };

            // 获得当前上下文
            EventContext &getContext(Event event);
            /**
             * @brief 重置事件上下文
             * @param[in, out] ctx 待重置的上下文类
             */
            void resetContext(EventContext &ctx);
            /**
             * @brief 触发事件
             * @param[in] event 事件类型
             */
            void triggerEvent(Event event);

            EventContext read;    // 读事件
            EventContext write;   // 写事件
            int fd = 0;           // 事件关联的句柄
            Event events = NONE; // 事件状态
            MutexType mutex;
        };

    public:
        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否将调用线程包含进去
         * @param[in] name 调度器的名称
         */
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        ~IOManager();

        // 0 success -1 error
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        // 删除事件
        bool delEvent(int fd, Event event);
        // 取消事件， 将事件需要一定条件要出发，并将该事件强制触发掉
        bool cancelEvent(int fd, Event event);
        // 取消一个句柄下的全部事件
        bool cancelAll(int fd);
        // 获取当前的 IOManager
        static IOManager *GetThis();

    protected:
        // 实现 父类的 虚方法
        /*
            如果派生类在虚函数声明时使用了override描述符，那么该函数必须重载其基类中的同名函数，否则代码将无法通过编译。
            如果派生类里面是像重载虚函数 就加上关键字override 这样编译器可以辅助检查是不是正确重载，
            如果没加这个关键字 也没什么严重的error 只是少了编译器检查的安全性
        */
        void tickle() override;
        bool stopping() override;
        void idle() override;

        void contextResize(size_t size);
        void onTimerInsertedAtFront() override;
        bool stopping(uint64_t& timeout);

    private:
        // epoll 文件句柄
        int m_epfd = 0;
        // 分别代表了 pipe(管道通信)中的两端，一般一端用于read一端用于write
        int m_tickleFds[2];
        // 记录了当前在等待执行的事件数量
        std::atomic<size_t> m_pendingEventCount = {0};
        RWMutexType m_mutex;
        // socket 事件上下文的容器
        std::vector<FdContext *> m_fdContexts;
    };
}
#endif