
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sendian.h"
#include "address.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    // 创建掩码
    template<class T>
    static T CreateMask(uint32_t bits)
    {
        return (1 << (sizeof(T) * 8 - bits)) - 1;
    }


    int Address::geFamily() const
    {
        return getAddr()->sa_family;
    }
    std::string Address::toString()
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    // 重写 operator
    bool Address::operator<(const Address& rhs) const
    {
        socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
        // 把str1 和str2 的前 n 个字节进行比较
        /*
            如果返回值 < 0，则表示 str1 小于 str2。
            如果返回值 > 0，则表示 str2 小于 str1。
            如果返回值 = 0，则表示 str1 等于 str2。
        */
        int result = memcmp(getAddr(), rhs.getAddr(), minlen);
        if (result < 0)
        {
            return true;
        }
        else if (result > 0)
        {
            return false;
        }
        else if (getAddrLen() < rhs.getAddrLen())
        {
            return true;
        }
        return false;

    }
    bool Address::operator==(const Address& rhs) const
    {
        return getAddrLen() == rhs.getAddrLen() 
               && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }
    bool Address::operator!=(const Address& rhs) const
    {
        return !(*this == rhs);
    }
    
    // IPAddress
    IPAddress::ptr IPAddress::Create(const char* address, uint32_t port = 0)
    {
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(addrinfo));

        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;
        int error = getaddrinfo(address, NULL, &hints, &results);
        if(error) {
            SYLAR_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
                << ", " << port << ") error=" << error
                << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                    Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if(result) {
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
        } catch (...) {
            freeaddrinfo(results);
            return nullptr;
        }
    }


    // ipv4
    IPv4Address::ptr IPv4Address::Create(const char* address, uint32_t port)
    {
        IPv4Address::ptr rt(new IPv4Address);
        rt->m_addr.sin_port = byteswapOnLittleEndian(port);
        // 将本地字节序转化为网络字节序
        int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
        if (result <= 0)
        {
            SYLAR_LOG_INFO(g_logger) << "IPv4Address::Create(" << address << ", "
                                     << port << ") rt=" << " errno=" << errno
                                     << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    } 

    IPv4Address::IPv4Address(const sockaddr_in& address)
    {
        m_addr = address;
    }

    IPv4Address::IPv4Address(uint32_t address, uint32_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteswapOnLittleEndian(port);
        m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
    }

    const sockaddr* IPv4Address::getAddr() const
    {
        return (sockaddr*)&m_addr;
    }
    socklen_t IPv4Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }
    std::ostream& IPv4Address::insert(std::ostream& os) const 
    {
        int32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
        // int 转 ipv4地址
        os << ((addr >> 24) & 0xff) << "."
           << ((addr >> 16) & 0xff) << "."
           << ((addr >> 8) & 0xff) << "."
           << (addr & 0xff);
        os << byteswapOnLittleEndian(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) 
    {
        if (prefix_len > 32)
        {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        // 得到掩码
        baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len)); 
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) 
    {
        if (prefix_len > 32)
        {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        // 得到掩码
        baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len)); 
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) 
    {
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    uint32_t IPv4Address::getPort() const 
    {
        return byteswapOnLittleEndian(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint32_t v) 
    {
        m_addr.sin_port = byteswapOnLittleEndian(v);
    }

    // ipv6
    IPv6Address::ptr IPv6Address::Create(const char* address, uint32_t port)
    {
        IPv6Address::ptr rt(new IPv6Address);
        rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
        // 将本地字节序转化为网络字节序
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
        if (result <= 0)
        {
            SYLAR_LOG_INFO(g_logger) << "IPv4Address::Create(" << address << ", "
                                     << port << ") rt=" << " errno=" << errno
                                     << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }

    IPv6Address::IPv6Address()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;        
    }

    IPv6Address::IPv6Address(const sockaddr_in6& addr)
    {
        m_addr = addr;
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint32_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswapOnLittleEndian(port);
        memcpy(&m_addr.sin6_addr.s6_addr, address, 16);

    }
    const sockaddr* IPv6Address::getAddr() const
    {
        return (sockaddr*)&m_addr;
    }
    socklen_t IPv6Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }
    std::ostream& IPv6Address::insert(std::ostream& os) const 
    {
        os << "[";
        uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
        bool userd_zeros = false;
        for(size_t i = 0; i < 8; ++i)
        {
            if (addr[i] == 0 && !userd_zeros)
            {
                continue;
            }
            if (i && addr[i - 1] == 0 && !userd_zeros)
            {
                os << ":";
                userd_zeros = true;
            }
            if (i)
            {
                os << ":";
            }
            // 输出16进制的
            os << std::hex << byteswapOnLittleEndian(addr[i]) << std::dec;
        }
        if (!userd_zeros && addr[7] == 0)
        {
            os << "::";
        }
        // ipv6 是每两个字节显示16进制的数字
        os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
        return os;
        
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) 
    {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= 
            CreateMask<uint8_t>(prefix_len % 8);
        for (int i = prefix_len / 8 + 1; i < 16; ++i)
        {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) 
    {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &= 
            CreateMask<uint8_t>(prefix_len % 8);

        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) 
    {
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len / 8] = 
                ~CreateMask<uint8_t>(prefix_len % 8);
        for (uint32_t i = 0; i < prefix_len / 8; ++i)
        {
            subnet.sin6_addr.s6_addr[i] = 0XFF;
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint32_t IPv6Address::getPort() const 
    {
        return byteswapOnLittleEndian(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint32_t v) 
    {
        m_addr.sin6_port = v;
    }

    // unix
    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

    UnixAddress::UnixAddress()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        /*
        offsetof(s,v)
        它的第一个参数是一个结构体，第二个参数是这个结构体中变量的名字。
        这个宏会返回那个变量距结构体头部的字节偏移量(Byte Offset)
        */
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string& path)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;
        if (!path.empty() && path[0] == '\0')
        {
            --m_length;
        }

        if (m_length > sizeof(m_addr.sun_path))
        {
            throw std::logic_error("path too long");
        }
        memcpy(m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);

    }

    const sockaddr* UnixAddress::getAddr() const 
    {
        return (sockaddr*)&m_addr;
    }
    socklen_t UnixAddress::getAddrLen() const 
    {
        return m_length;
    }
    std::ostream& UnixAddress::insert(std::ostream& os) const 
    {
        if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0')
        {
            return os << "\\0" << std::string(m_addr.sun_path + 1,
                    m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_addr.sun_path;
    }

    // unknow
    UnknowAddress::UnknowAddress(int family)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }
    UnknowAddress::UnknowAddress(const sockaddr& addr)
    {
        m_addr = addr;
    }

    const sockaddr* UnknowAddress::getAddr() const 
    {
        return &m_addr;
    }
    socklen_t UnknowAddress::getAddrLen() const 
    {
        return sizeof(m_addr);
    }
    std::ostream& UnknowAddress::insert(std::ostream& os) const 
    {
        os << "[UnknowAddress family = " << m_addr.sa_family << "]";
        return os;
    }

}