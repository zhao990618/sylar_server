#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <functional>
#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"
namespace sylar
{
    // 拿到指向自己的指针
    class TcpServer : public std::enable_shared_from_this<TcpServer>
                        ,Noncopyable
    {
        public:
            typedef std::shared_ptr<TcpServer> ptr;
            TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis()
                     ,sylar::IOManager* io_woker = sylar::IOManager::GetThis()
                     ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());
            virtual ~TcpServer();

            // 一个地址的bind
            virtual bool bind(sylar::Address::ptr addr, bool ssl = false);
            // 一堆地址的bind
            virtual bool bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails
                        ,bool ssl = false);
            // 服务器的启动
            virtual bool start();
            // 服务器的停止
            virtual void stop();

            uint64_t getRecvTimeout() const {return m_recvTimeout;}
            /**
             * @brief 返回服务器名称
             */
            std::string getName() const { return m_name;}

            /**
             * @brief 设置读取超时时间(毫秒)
             */
            void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}

            /**
             * @brief 设置服务器名称
             */
            virtual void setName(const std::string& v) { m_name = v;}

            /**
             * @brief 是否停止
             */
            bool isStop() const { return m_isStop;}

        protected:
            /**
             * @brief 处理新连接的Socket类
             */
            virtual void handleClient(Socket::ptr client);
                        /**
             * @brief 开始接受连接
             */
            virtual void startAccept(Socket::ptr sock);
        protected:
            /// 监听Socket数组
            std::vector<Socket::ptr> m_socks;
            /// 新连接的Socket工作的调度器
            IOManager* m_worker;
            IOManager* m_ioWorker;
            /// 服务器Socket接收连接的调度器
            IOManager* m_acceptWorker;
            /// 接收超时时间(毫秒)
            uint64_t m_recvTimeout;
            /// 服务器名称
            std::string m_name;
            /// 服务器类型
            std::string m_type = "tcp";
            /// 服务是否停止
            bool m_isStop;

            bool m_ssl = false;

            // TcpServerConf::ptr m_conf;
    };
}

#endif