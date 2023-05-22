#pragma once

#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
// 智能指针的操作
#include <memory>
#include <string>
#include <pthread.h>
// #include "mutex.h"
#include "noncopyable.h"
#include <atomic>
#include <stdint.h>
#include <semaphore.h>


namespace sylar
{

    class Semaphore : Noncopyable
    {
    public:
        // 因为 count 为0,说明当前只能有1个线程使用信号量
        Semaphore(uint32_t count = 0);
        ~Semaphore();
        void wait();
        void notify();
        // sem_t getSemaphore() const {return m_semaphore;}

    private:
        int ret;
        sem_t m_semaphore;
    };

    template <class T>
    class ScopedLockImpl
    {
    public:
       ScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    template <class T>
    class ReadScopedLockImpl
    {
    public:
        ReadScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopedLockImpl()
        {
            unlock();
        }

        void rdlock()
        {
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };


    template <class T>
    class WriteScopedLockImpl
    {
    public:
        WriteScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopedLockImpl()
        {
            unlock();
        }

        void wrlock()
        {
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    class Mutex : Noncopyable
    {
        public:
            typedef ScopedLockImpl<Mutex> Lock;

            Mutex()
            {
                pthread_mutex_init(&m_mutex, nullptr);
            }
            
            ~Mutex()
            {
                pthread_mutex_destroy(&m_mutex);
            }

            void lock()
            {
                pthread_mutex_lock(&m_mutex);
            }

            void unlock()
            {
                pthread_mutex_unlock(&m_mutex);
            }

        private:
            pthread_mutex_t m_mutex;
    };

    class SpinLock : Noncopyable
    {
        public:
            typedef ScopedLockImpl<SpinLock> Lock;
            SpinLock()
            {
                pthread_spin_init(&m_mutex, 0);    
            }
            
            ~SpinLock()
            {
                pthread_spin_destroy(&m_mutex);
            }

            void lock()
            {
                pthread_spin_lock(&m_mutex);
            }

            void unlock()
            {
                pthread_spin_unlock(&m_mutex);
            }
        private:
            pthread_spinlock_t m_mutex;
    };


    // 读写锁
    class RWMutex : Noncopyable
    {   
        public:
            typedef ReadScopedLockImpl<RWMutex> ReadLock;
            typedef WriteScopedLockImpl<RWMutex> WriteLock;
            RWMutex()
            {
                pthread_rwlock_init(&m_lock, nullptr);
            }

            ~RWMutex()
            {
                pthread_rwlock_destroy(&m_lock);
            }

            // 读锁
            void rdlock()
            {
                pthread_rwlock_rdlock(&m_lock);
            }

            // 写锁
            void wrlock()
            {
                pthread_rwlock_wrlock(&m_lock);
            }

            // 解锁
            void unlock()
            {
                pthread_rwlock_unlock(&m_lock);
            }

        private:
            pthread_rwlock_t m_lock;
    };


    class Thread
    {
    public:
        typedef std::shared_ptr<Thread> ptr;
        Thread(std::function<void()> cb, const std::string &name);
        ~Thread();
        pid_t getId() const { return m_id; }
        const std::string &getName() const { return m_name; }
        void join();
        // 获得当前线程引用
        static Thread *GetThis();
        // 获取当前线程的名称
        static const std::string &GetName();
        // 应为是静态的 所以在 构造函数的时候就已经执行了 SetName(name)
        static void SetName(const std::string &name);

    private:
        // 禁止默认copy
        Thread(const Thread &) = delete;
        Thread(const Thread &&) = delete;
        Thread &operator=(const Thread &) = delete;

        static void *run(void *arg);

    private:
        // 线程id
        pid_t m_id = -1;
        pthread_t m_thread = 0;
        // 返回值为void, 无参数
        std::function<void()> m_cb;
        std::string m_name;
        // 信号量
        Semaphore m_semaphore;
    };
}

#endif