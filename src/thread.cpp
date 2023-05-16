#include "thread.h"
#include "log.h"
#include "util.h"
#include <iostream>

namespace sylar
{
    // 线程局部指针 -- 指向当前线程
    /* thread_local 修饰的对象在 前线程创建时生成，在线程销毁是灭亡；
        thread_local 一般用于需要保证线程安全的函数中，
        如果类的成员函数中定义了 thread_local变量，则对于
        同一个线程 内的该类的多个对象都会共享一个变量实例 --
    */
    static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOW";

    // 系统日志统一 用system
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    Semaphore::Semaphore(uint32_t count)
    {
        // initialize an unnamed semaphore
        // int sem_init(sem_t *sem, int pshared, unsigned int value);
        // The value argument specifies the initial value for  the  semaphore.

        // // 返回0 表示成功, pshared = 0 说明进程内的线程共享该信号量；否则就是进程间共享
        if (sem_init(&m_semaphore, 0, count)) // 而count == 0即线程自身使用信号量
        {
            perror("sem_init");
            throw std::logic_error("sem_init error");
        }
    }
    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }
    void Semaphore::wait()
    {
        // 返回0 表示成功
        if (sem_wait(&m_semaphore)) // 进入新的线程，阻塞当前线程（wait），
        {
            perror("sem_wait");
            throw std::logic_error("sem_wait error");
        }
    }
    void Semaphore::notify()
    {
        if (sem_post(&m_semaphore)) // +1，让当前线程阻塞，并且原先阻塞的线程释放（post），
        {
            perror("sem_post");
            throw std::logic_error("sem_post error");
        }
    }




    // 获得当前线程引用
    Thread* Thread::GetThis()
    {
        return t_thread;
    }

    // 获取当前线程的名称
    const std::string& Thread::GetName()
    {
        return t_thread_name;
    }

    void Thread::SetName(const std::string& name)
    {
        if (name.empty())
        {
            return;
        }
        if (t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    // void* 是一个未定义步长的指针，可以和任意类型指针相等
    // 因为run是static函数，编译器不会将对象地址(this)作为隐参数传入，所以需要手动传入this
    void* Thread::run(void* arg)
    {
        // run函数的参数是 this， 即将这个对象自身传入作为void* arg
        Thread* thread = (Thread*)arg;
        // 让局部指针 t_thread 指向当前对象
        t_thread = thread;
        // 线程名字
        t_thread_name = thread->m_name;
        // 获得线程id
        thread->m_id = sylar::GetThreadId();
        // 为当前线程命名
        pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());
        std::function<void()> cb;
        // 将函数中的智能指针是放掉
        cb.swap(thread->m_cb);
        // SYLAR_LOG_INFO(g_logger) << "post start";
        // 信号量+1, 在当前信号量上的所有阻塞线程进行争抢，只有一个线程可以抢到执行资源,变成不阻塞状态
        thread->m_semaphore.notify();
        // SYLAR_LOG_INFO(g_logger) << "post end";
        // 抢到执行资源的线程可以 执行cb()
        cb();
        return 0;
    }

    Thread::Thread(std::function<void()> cb, const std::string& name)
        :m_cb(cb)
        ,m_name(name)
    {
        if (name.empty())
        {
            m_name = "UNKNOW";
        }
        // this 指向当前对象，当前对象的地址作为隐含参数，在对象执行函数时
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if (rt)
        {
            perror("pthread_create");
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt =" << rt   
                                      << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        // 等待信号量
        // 使得当前线程 等待， 直到sem > 0, 才获得执行资源
        // SYLAR_LOG_INFO(g_logger) << "wait start";
        m_semaphore.wait();
        // SYLAR_LOG_INFO(g_logger) << "wait end";
    }
    
    Thread::~Thread()
    {
        if (m_thread)
        {
            /*
            一般情况下，线程终止后，其终止状态一直保留到其它线程调用pthread_join获取它的状态为止。
            但是线程也可以被置为detach状态，这样的线程一旦终止就立刻回收它占用的所有资源，
            而不保留终止状态。不能对一个已经处于detach状态的线程调用pthread_join，
            这样的调用将返回EINVAL错误。
            */
            pthread_detach(m_thread); // 线程分离
        }
    }
    
    void Thread::join()
    {
        if (m_thread)
        {
            int rt = pthread_join(m_thread, nullptr);
            if (rt)
            {
                perror("pthread_join");
                SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt =" << rt   
                                          << " name=" << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }


}