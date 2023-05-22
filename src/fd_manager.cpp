#include "fd_manager.h"
#include "sys/stat.h"
#include "sys/types.h"
#include <unistd.h>
#include "hook.h"

/*
    判断传进来的句柄fd是否是socket的
    如果是socket的那就需要用hook的函数
    如果不是那就按照原来的函数执行
*/

namespace sylar
{

    FdCtx::FdCtx(int fd)
        : m_isInit(false), m_isSocket(false), m_sysNonblock(false), m_userNonblock(false), m_isClosed(false), m_fd(fd), m_recvTimeout(-1), m_sendTimeout(-1)
    {
        init();
    }
    FdCtx::~FdCtx()
    {
    }

    bool FdCtx::init()
    {
        // 如果初始化就返回true
        if (m_isInit)
        {
            return true;
        }
        // 没有初始化就设置为默认状态
        m_recvTimeout = -1;
        m_sendTimeout = -1;

        // 可以用来判断fd是文件的信息，判断是否是文件描述符
        struct stat fd_stat;
        // fstat 作用在于将m_fd 所指向的文件状态复制到fd_stat中
        // 失败返回-1
        if (-1 == fstat(m_fd, &fd_stat))
        {
            m_isInit = false;
            m_isSocket = false;
        }
        else
        {
            m_isInit = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }

        // 如果 是socket描述符
        if (m_isSocket)
        {
            // 获得fd的状态是否为阻塞
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            /*
                判断flags与O_NONBLOCK是否一样，
                如果一样就是阻塞，不一样就进入if并且修改状态为O_NONBLOCK
            */
            if (!(flags & O_NONBLOCK))
            {
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
            m_sysNonblock = true;
        }
        else
        {
            m_sysNonblock = false;
        }

        m_userNonblock = false;
        m_isClosed = false;
        return m_isInit;
    }

    void FdCtx::setTimeout(int type, uint64_t v)
    {
        if (type == SO_RCVTIMEO)
        {
            m_recvTimeout = v;
        }
        else
        {
            m_sendTimeout = v;
        }
    }
    uint64_t FdCtx::getTimeout(int type)
    {
        if (type == SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        else
        {
            return m_sendTimeout;
        }
    }

    FdManager::FdManager()
    {
        m_datas.resize(64);
    }

    FdCtx::ptr FdManager::get(int fd, bool auto_create)
    {
        if (fd == -1)
        {
            return nullptr;
        }
        RWMutexType::ReadLock lock(m_mutex);
        // 说明fd不在m_datas 的范围内 m_datas.resize(64)
        if ((int)m_datas.size() <= fd)
        {
            // 不需要自动创建FdCtx
            if (auto_create == false)
            {
                return nullptr;
            }
        }
        else
        { // 说明fd在m_datas 的范围内 m_datas.resize(64);并且不需要创建FdCtx
            if (m_datas[fd] || !auto_create)
            {
                return m_datas[fd];
            }
        }
        // 需要创建FdCtx并且fd在m_datas的范围内
        lock.unlock();

        RWMutexType::WriteLock lock2(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if (fd >= (int)m_datas.size())
        {
            m_datas.resize(fd * 1.5);
        }
        m_datas[fd] = ctx;
        return ctx;
    }
    void FdManager::del(int fd)
    {
        RWMutexType::WriteLock lock(m_mutex);
        // 即fd已经不存在于 m_datas中
        if ((int)m_datas.size() <= fd)
        {
            return;
        }
        m_datas[fd].reset();
    }
}