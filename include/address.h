#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <memory>
#include <string>
#include <netinet/in.h>
#include <iostream>
#include <sstream>

namespace sylar
{
    class Address
    {
        public:
            typedef std::shared_ptr<Address> ptr;

            // 域名转化地址
            static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

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
            
            static IPAddress::ptr Create(const char* address, uint32_t port = 0);

            // 拿到广播地址
            virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
            // 网络地址
            virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
            // 子网掩码
            virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
            // 得到端口号
            virtual uint32_t getPort() const = 0;
            // 设置端口号
            virtual void setPort(uint32_t v) = 0; 
    };

    class IPv4Address : public IPAddress
    {
        public:
            typedef std::shared_ptr<IPv4Address> ptr;
            // 将文本型的地址转化为 address(IPv4)
            static IPv4Address::ptr Create(const char* address, uint32_t port = 0);
            IPv4Address(const sockaddr_in& address);
            IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;
            // 拿到广播地址
            IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
            // 网络地址
            IPAddress::ptr networkAddress(uint32_t prefix_len) override;
            // 子网掩码
            IPAddress::ptr subnetMask(uint32_t prefix_len) override;
            // 得到端口号
            uint32_t getPort() const override;
            // 设置端口号
            void setPort(uint32_t v) override;
        private:
            sockaddr_in m_addr;
    };

    class IPv6Address : public IPAddress
    {
        public:
            typedef std::shared_ptr<IPv6Address> ptr;
            static IPv6Address::ptr Create(const char* address, uint32_t port = 0);
            IPv6Address();
            IPv6Address(const sockaddr_in6& addr);
            IPv6Address(const uint8_t address[16], uint32_t port);
            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;
            // 拿到广播地址
            IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
            // 网络地址
            IPAddress::ptr networkAddress(uint32_t prefix_len) override;
            // 子网掩码
            IPAddress::ptr subnetMask(uint32_t prefix_len) override;
            // 得到端口号
            uint32_t getPort() const override;
            // 设置端口号
            void setPort(uint32_t v) override;
        private:
            sockaddr_in6 m_addr;
    };

    class UnixAddress : public Address
    {
        public:
            typedef std::shared_ptr<UnixAddress> ptr;
            UnixAddress();
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
            UnknowAddress(int family);
            UnknowAddress(const sockaddr& addr);

            const sockaddr* getAddr() const override;
            socklen_t getAddrLen() const override;
            std::ostream& insert(std::ostream& os) const override;

        private:
            struct sockaddr m_addr;       
    };
}

#endif