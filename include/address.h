#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <iostream>

namespace sylar
{
    class Address
    {
        public:

            typedef std::shared_ptr<Address> ptr;
            virtual ~Address() {};
            // 得到协议
            int geFamily() const;

            virtual const sockaddr* getAddr() const = 0;
            virtual socklen_t getAddrLen() const = 0;

            virtual std::ostream& insert(std::ostream& os) const = 0;
            std::string toString();

            // 重写 operator
            bool operator<(const Address& rhs) const;
            bool operator==(const Address& rhs) const;
            bool operator!=(const Address& rhs) const;
    };
    
    class IPAddress : public Address
    {
        public:
            typedef std::shared_ptr<IPAddress> ptr;
            
            // 拿到广播地址
            virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
            // 网络地址
            virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
            // 子网掩码
            virtual IPAddress::ptr subnetAddress(uint32_t prefix_len) = 0;
            // 得到端口号
            virtual int32_t getPort() const = 0;
            // 设置端口号
            virtual void setPort(uint32_t v) = 0; 
    };

    class IPv4Address : public IPAddress
    {
        public:
            typedef std::shared_ptr<IPv4Address> ptr;

            IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;
            // 拿到广播地址
            IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
            // 网络地址
            IPAddress::ptr networkAddress(uint32_t prefix_len) override;
            // 子网掩码
            IPAddress::ptr subnetAddress(uint32_t prefix_len) override;
            // 得到端口号
            int32_t getPort() const override;
            // 设置端口号
            void setPort(uint32_t v) override;
        private:
            sockaddr_in m_addr;
    };

    class IPv6Address : public IPAddress
    {
        public:
            typedef std::shared_ptr<IPv6Address> ptr;

            IPv6Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;
            // 拿到广播地址
            IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
            // 网络地址
            IPAddress::ptr networkAddress(uint32_t prefix_len) override;
            // 子网掩码
            IPAddress::ptr subnetAddress(uint32_t prefix_len) override;
            // 得到端口号
            int32_t getPort() const override;
            // 设置端口号
            void setPort(uint32_t v) override;
        private:
            sockaddr_in6 m_addr;
    };

    class UnixAddress : public Address
    {
        public:
            typedef std::shared_ptr<UnixAddress> ptr;
            UnixAddress(const std::string& path);

            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;

        private:
            struct sockaddr_un m_addr;
            socklen_t m_length;
    };

    class UnknowAddress : public Address
    {
        public:
            typedef std::shared_ptr<UnknowAddress> ptr;
            UnknowAddress(const std::string& path);

            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;

        private:
            struct sockaddr_un m_addr;       
    };
}

#endif