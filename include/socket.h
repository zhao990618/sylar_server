#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include <memory>
#include "address.h"
#include "noncopyable.h"

namespace sylar
{
    class Socket : public std::enable_shared_from_this<Socket>, Noncopyable
    {
        public:
            // shared_ptr 可以控制所指向内存的释放，当shared_ptr要被释放时，其所指向的内存空间也要被释放
            // 会出现内存泄漏的可能，两个shared_ptr相互指向对方
            typedef std::shared_ptr<Socket> ptr;
            // weak_ptr 不可以控制所指向内存的释放，连引用计数也不会+1，所以不会存在内存泄露
            typedef std::weak_ptr<Socket> weak_ptr;

            enum Type
            {
                TCP = SOCK_STREAM,
                UDP = SOCK_DGRAM
            };

            enum Family
            {
                IPv4 = AF_INET,
                IPv6 = AF_INET6,
                UNIX = AF_UNIX
            };

            static Socket::ptr CreateTCP(sylar::Address::ptr address);
            static Socket::ptr CreateUDP(sylar::Address::ptr address);

            static Socket::ptr CreateTCPSocket();
            static Socket::ptr CreateUDPSocket();

            static Socket::ptr CreateTCPSocket6();
            static Socket::ptr CreateUDPSocket6();

            static Socket::ptr CreateUnixTCPSocket();
            static Socket::ptr CreateUnixUDPSocket();

            Socket(int family, int type, int protocol = 0);
            ~Socket();

            // 设置和得到 发送超时时间
            int64_t getSendTimeout();
            void setSentTimeout(int64_t v);

            // 设置和得到 接受超时时间
            int64_t getRecvTimeout();
            void setRecvTimeout(int64_t v);

            // 获取socket句柄上的一些信息
            bool getOption(int level, int option, void* result, size_t* len);
            template<class T>
            bool getOption(int level, int option, T& result)
            {
                size_t length = sizeof(T);
                return getOption(level, option, result, &length);
            }

            // 设置socket句柄上的一些信息
            bool setOption(int level, int option, const void* value, size_t len);
            template<class T>
            bool setOption(int level, int option, const T& value)
            {
                return setOption(level, option, &value, sizeof(T));
            }

            Socket::ptr accept();

            bool bind(const Address::ptr addr);
            bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
            bool listen(int backlog = SOMAXCONN);
            bool close();

            int send(const void* buffer, size_t length, int flags = 0);
            int send(const iovec* buffer, size_t length, int flags = 0);
            int sendTo(const void* buffers, size_t length, const Address::ptr to, int flags = 0);
            int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

            int recv(void* buffer, size_t length, int flags = 0);
            int recv(iovec* buffer, size_t length, int flags = 0);
            int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
            int recvFrom(iovec* buffer, size_t length, Address::ptr from, int flags = 0);
            
            // 得到远端的ip
            Address::ptr getRemoteAddress();
            // 得到本地的ip
            Address::ptr getLocalAddress();

            // 得到socket一些配置属性
            int getFamily() const
            {
                return m_family;
            }
            int getType() const
            {
                return m_type;
            }
            int getProtocol() const
            {
                return m_protocol;
            }

            bool isConnected() const
            {
                return m_isConnected;
            }
            int getSocket() const 
            {
                return m_sock;
            }
            bool isValid() const;
            int getError();

            std::ostream& dump(std::ostream& os) const;

            std::string toString() const;
            bool cancelRead();
            bool cancelWrite();
            bool cancelAccept();
            bool cancelAll();
        private:
            void initSock();
            void newSock();
            bool init(int sock);
        private:
            // socket句柄
            int m_sock;
            int m_family;   // IPv4 IPv6
            int m_type;     // TCP UDP
            int m_protocol; // 协议
            int m_isConnected;

            Address::ptr m_remoteAddress;
            Address::ptr m_localAddress;

    };
}

#endif