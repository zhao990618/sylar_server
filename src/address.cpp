
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

    // 通过翻转最右侧的1开始所有位数，来达到计算有多少个1
    template<class T>
    static uint32_t CountBytes(T value)
    {
        uint32_t result = 0;
        for(; value; result++)
        {
            value &= value - 1;
        }
        return result;
    }

    int Address::getFamily() const
    {
        return getAddr()->sa_family;
    }

    std::string Address::toString() const
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol)
    {
        std::vector<Address::ptr> results;
        // 如果返回值是true，那就是有值的
        if (Lookup(results, host, type, protocol))
        {
            return results[0];
        }
        return nullptr;
    }

    IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol)
    {
        std::vector<Address::ptr> results;
        // 如果返回值是true，那就是有值的
        if (Lookup(results, host, type, protocol))
        {
            for (auto& i : results)
            {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if (v)
                {
                    return v;
                }
            }
        }
        return nullptr;
    }

    bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                    int family, int type, int protocol)
    {
        addrinfo hints, *results, *next;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string node;
        const char* service = NULL;
        // ipv6允许有[],[]里面:之后的就是地址
        // 检查ipv6address service
        if (!host.empty() && host[0] == '[')
        {
            // void* 类型
            /*
                c_str() 返回当前字符串首地址 host.c_str() + 1是因为host的首地址存的是[
                void *memchr(const void *str, int c, size_t n) 
                    参数str所指向的字符串的前n个字节中搜索第一次出现字符c
            */
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
            if (endipv6)
            {
                // 如果后一个字符值 等于 ':'
                if (*(endipv6 + 1) == ':')
                {
                    // service 等于':'之后的值
                    service = endipv6 + 2;
                }
                // ipv6的地址
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        // 检查 node service
        if (node.empty())
        {
            // 从指定的字符串中返回目标字符地址
            service = (const char*)memchr(host.c_str(), ':', host.size());
            if (service)
            {
                // 从当前找到的':'后一个位置开始找，继续往后找
                // 如果后续不存在':'
                if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1))
                {
                    // 第一个字节地址到 第一个':'中间的字符表示为 ipv6的地址
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        }

        // host 里面没有 ':'
        if (node.empty())
        {
            node = host;
        }
        // 函数将主机名、主机地址、服务名和端口的字符串表示转换成套接字地址结构体
        int error = getaddrinfo(node.c_str(), service, &hints, &results);
        if(error) {
            SYLAR_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
                << family << ", " << type << ") err=" << error << " errstr="
                << gai_strerror(error);
            return false;
        }

        next = results;
        while(next)
        {
            // 得到addr --->  sockaddr * addrinfo::ai_addr
            result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
            // 到下一个结构体
            next = next->ai_next;
        }
        freeaddrinfo(results);
        return !result.empty();
    }


    bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result,
                        int family) {
        struct ifaddrs *next, *results;
        // 得到本地的 地址
        if(getifaddrs(&results) != 0) {
            SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                " err=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        try {
            for(next = results; next; next = next->ifa_next) {
                Address::ptr addr;
                uint32_t prefix_len = ~0u;
                if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                    continue;
                }
                switch(next->ifa_addr->sa_family) {
                    case AF_INET:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                            uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                            prefix_len = CountBytes(netmask);
                        }
                        break;
                    case AF_INET6:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                            in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                            // ipv6 有16个，那就循环遍历16个字节
                            prefix_len = 0;
                            for(int i = 0; i < 16; ++i) {
                                prefix_len += CountBytes(netmask.s6_addr[i]);
                            }
                        }
                        break;
                    default:
                        break;
                }

                if(addr) {
                    result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
                }
            }
        } catch (...) {
            SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
            freeifaddrs(results);
            return false;
        }
        freeifaddrs(results);
        return !result.empty();
    }

    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                        ,const std::string& iface, int family) {
        if(iface.empty() || iface == "*") {
            if(family == AF_INET || family == AF_UNSPEC) {
                result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
            }
            if(family == AF_INET6 || family == AF_UNSPEC) {
                result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
            }
            return true;
        }

        std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

        if(!GetInterfaceAddresses(results, family)) {
            return false;
        }
        
        // 找到等于iface所对应的迭代器
        // its->first == pair<next->ifa_name, std::make_pair(addr, prefix_len)>
        auto its = results.equal_range(iface);
        for(; its.first != its.second; ++its.first) {
            result.push_back(its.first->second);
        }
        return !result.empty();
    }

    Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen)
    {
        if (addr == nullptr)
        {
            return nullptr;
        }

        Address::ptr result;
        switch (addr->sa_family)
        {
        case AF_INET:
            /* code */
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknowAddress(*addr));
            break;
        }
        return result;
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
    IPAddress::ptr IPAddress::Create(const char* address, uint16_t port)
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
            // 父类指针指向子类个体
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                    Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if(result) {
                result->setPort(port);
            }
            // 释放掉results
            freeaddrinfo(results);
            return result;
        } catch (...) {
            freeaddrinfo(results);
            return nullptr;
        }
    }


    // ipv4
    IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port)
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

    IPv4Address::IPv4Address(uint32_t address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteswapOnLittleEndian(port);
        m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
    }

    sockaddr* IPv4Address::getAddr() 
    {
        return (sockaddr*)&m_addr;
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
        os << ":"<< byteswapOnLittleEndian(m_addr.sin_port);
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

    IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) 
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

    void IPv4Address::setPort(uint16_t v) 
    {
        m_addr.sin_port = byteswapOnLittleEndian(v);
    }

    // ipv6
    IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port)
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

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswapOnLittleEndian(port);
        memcpy(&m_addr.sin6_addr.s6_addr, address, 16);

    }

    sockaddr* IPv6Address::getAddr() {
        return (sockaddr*)&m_addr;
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

    IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) 
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

    void IPv6Address::setPort(uint16_t v) 
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
    
    sockaddr* UnixAddress::getAddr() {
        return (sockaddr*)&m_addr;
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

    sockaddr* UnknowAddress::getAddr() {
        return (sockaddr*)&m_addr;
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
    std::ostream& operator<<(std::ostream& os, const Address& addr) {
        return addr.insert(os);
    }
}