#pragma onece
#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>

namespace sylar
{
#if 0
    class Semaphore
    {
    public:
        // 因为 count 为0,说明当前只能有1个线程使用信号量
        Semaphore(uint32_t count = 0);
        ~Semaphore();
        void wait();
        void notify();
        // sem_t getSemaphore() const {return m_semaphore;}

    private:
        Semaphore(const Semaphore &) = delete;
        Semaphore(const Semaphore &&) = delete;
        Semaphore &operator=(const Semaphore &) = delete;

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

    class Mutex
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

    class SpinLock
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

#if 0
    class NullMutex
    {
        public:
            typedef ScopedLockImpl<NullMutex> Lock;
            NullMutex(){}
            ~NullMutex(){}
            void lock() {}
            void unlock(){}
        private:
            pthread_mutex_t m_mutex;
    };

        class NullRWMutex
    {   
        public:
            typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
            typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
            NullRWMutex(){}
            ~NullRWMutex(){}
            // 读锁
            void rdlock(){}
            // 写锁
            void wrlock(){}
            // 解锁
            void unlock(){}
        private:
            pthread_rwlock_t m_lock;
    };
#endif

    // 读写锁
    class RWMutex
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
#endif
}
#endif