#include "address.h"
#include <sstream>

namespace sylar
{
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
    // ipv4
    IPv4Address::IPv4Address(uint32_t address, uint32_t port)
    {

    }
    const sockaddr* IPv4Address::getAddr() const
    {

    }
    socklen_t IPv4Address::getAddrLen() const
    {

    }
    std::ostream& IPv4Address::insert(std::ostream& os) const 
    {

    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) 
    {

    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) 
    {

    }

    IPAddress::ptr IPv4Address::subnetAddress(uint32_t prefix_len) 
    {

    }

    int32_t IPv4Address::getPort() const 
    {

    }

    void IPv4Address::setPort(uint32_t v) 
    {

    }
    // ipv6
    IPv6Address::IPv6Address(uint32_t address, uint32_t port)
    {

    }
    const sockaddr* IPv6Address::getAddr() const
    {

    }
    socklen_t IPv6Address::getAddrLen() const
    {

    }
    std::ostream& IPv6Address::insert(std::ostream& os) const 
    {

    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) 
    {

    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) 
    {

    }

    IPAddress::ptr IPv6Address::subnetAddress(uint32_t prefix_len) 
    {

    }

    int32_t IPv6Address::getPort() const 
    {

    }

    void IPv6Address::setPort(uint32_t v) 
    {

    }

    // unix
    UnixAddress::UnixAddress(const std::string& path)
    {

    }

    const sockaddr* UnixAddress::getAddr() const 
    {

    }
    socklen_t UnixAddress::getAddrLen() const 
    {

    }
    std::ostream& UnixAddress::insert(std::ostream& os) const 
    {

    }

    // unknow
    UnknowAddress::UnknowAddress(const std::string& path)
    {

    }

    const sockaddr* UnknowAddress::getAddr() const 
    {

    }
    socklen_t UnknowAddress::getAddrLen() const 
    {

    }
    std::ostream& UnknowAddress::insert(std::ostream& os) const 
    {

    }

}