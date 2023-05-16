#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include <iomanager.h>
#include "thread.h"


namespace sylar
{
    /*
        每一个句柄fd 的类，每一个对象保存着一个句柄
        句柄分为filefd，和scokfd
    */
    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
        public:
            typedef std::shared_ptr<FdCtx> ptr;
            FdCtx(int fd);
            ~FdCtx();

            bool init();
            bool isInit() const {return m_isInit;}
            bool isSocket() const {return m_isSocket;}
            bool isClose() const {return m_isClosed;}
            bool close();

            void setUserNonblock(bool v) {m_userNonblock = v;}
            bool getUserNonblock() const {return m_userNonblock;}

            void setSysNonblock(bool v) {m_sysNonblock = v;}
            bool getSysNonblock() const {return m_sysNonblock;}

            void setTimeout(int type, uint64_t v);
            uint64_t getTimeout(int type);

        /* 针对文件描述符fd的状态解释 */
        private:
            // 是否初始化
            bool m_isInit: 1;
            // 是否是socket
            bool m_isSocket: 1;
            // 是否是系统设置的nonblock  
            bool m_sysNonblock: 1;
            // 是否是用户设置的nonblock
            bool m_userNonblock: 1;
            // 该文件描述符是否已经被关闭了
            bool m_isClosed: 1;
            int m_fd;
            // 接收超时时间
            uint64_t m_recvTimeout;
            // 发送超时时间
            uint64_t m_sendTimeout;
            // 句柄fd 与 io相关，需要一个iomanager的对象
            sylar::IOManager* m_iomanager;
    };

    /*
        Fdmanager 用来管理所有的句柄fd
    */
    class FdManager{
        public:
            typedef RWMutex RWMutexType;
            FdManager();
            /*
                获取fd所对应的FdCtx智能指针， 
                如果fd不存在(auto_create = true)，那么就自动创建一个FdCtx对象
            */ 
            FdCtx::ptr get(int fd, bool auto_create = false);
            void del(int fd);
        
        private:
            RWMutexType m_mutex;
            std::vector<FdCtx::ptr> m_datas;
    };
}

#endif