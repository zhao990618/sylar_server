#include "socket.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    
    Socket::ptr Socket::CreateTCP(sylar::Address::ptr address)
    {
        Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDP(sylar::Address::ptr address)
    {
        Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket()
    {
        Socket::ptr sock(new Socket(IPv4, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket()
    {
        Socket::ptr sock(new Socket(IPv4, UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket6()
    {
        Socket::ptr sock(new Socket(IPv6, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket6()
    {
        Socket::ptr sock(new Socket(IPv6, UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixTCPSocket()
    {
        Socket::ptr sock(new Socket(UNIX, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixUDPSocket()
    {
        Socket::ptr sock(new Socket(UNIX, UDP, 0));
        return sock;
    }
    
    // m_sock == -1 表示没有被初始化
    Socket::Socket(int family, int type, int protocol)
        :m_sock(-1)
        ,m_family(family)
        ,m_type(type)
        ,m_protocol(protocol)
        ,m_isConnected(false)
    {

    }
    Socket::~Socket()
    {
        close();
    }

    // 设置和得到 发送超时时间
    int64_t Socket::getSendTimeout()
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx)
        {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }
    void Socket::setSentTimeout(int64_t v)
    {
        struct timeval tv{int(v/1000), int(v % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    // 设置和得到 接受超时时间
    int64_t Socket::getRecvTimeout()
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx)
        {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }
    void Socket::setRecvTimeout(int64_t v)
    {
        struct timeval tv{int(v/1000), int(v % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    // 获取socket句柄上的一些信息
    bool Socket::getOption(int level, int option, void* result, size_t* len)
    {
        // 判断 套接字 是否处于 option选项状态 -- success 0； un -1
        int rt = getsockopt(m_sock, level, option, result, (socklen_t*) len);
        if (rt)
        {
            SYLAR_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
                << " level=" << level << " option=" << option
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }
    // 设置socket句柄上的一些信息
    bool Socket::setOption(int level, int option, const void* value, size_t len)
    {
        if(setsockopt(m_sock, level, option, value, (socklen_t)len)) {
            SYLAR_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
                << " level=" << level << " option=" << option
                << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept()
    {
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
        // :: 是全局作用域的意思，调用全局的accept
        int newsock = ::accept(m_sock, nullptr, nullptr);
        if(newsock == -1) {
            SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
            << errno << " errstr=" << strerror(errno);
            return nullptr;      
        }
        // 成功的句柄用来初始化
        if(sock->init(newsock)) {
            return sock;
        }
        return nullptr; 
    }

    // 对句柄做初始化
    bool Socket::init(int sock)
    {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
        if (ctx && ctx->isSocket() && !ctx->isClose())
        {
            m_sock = sock;
            m_isConnected = true;
            // 初始化一些地址
            getRemoteAddress();
            getLocalAddress();
            return true;
        }
        return false;
    }

    bool Socket::bind(const Address::ptr addr)
    {
        // m_sock 无效效
        if(!isValid())
        {
            newSock();
            if (SYLAR_UNLIKELY(!isValid()))
            {
                return false;
            }
        }

        // 如果连接的addr的类型和sock所对应的类型不一样
        if (SYLAR_UNLIKELY(addr->getFamily() != m_family))
        {
            SYLAR_LOG_ERROR(g_logger) << "bind sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        if (::bind(m_sock, addr->getAddr(), addr->getAddrLen()))
        {
            SYLAR_LOG_ERROR(g_logger) << "bind error errrno=" << errno
                << " errstr=" << strerror(errno);           
            return false;
        }
        getLocalAddress();
        return true;
    }
    
    bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
    {
        if(!isValid())
        {
            newSock();
            if (SYLAR_UNLIKELY(!isValid()))
            {
                return false;
            }
        }
        if (SYLAR_UNLIKELY(addr->getFamily() != m_family))
        {
            SYLAR_LOG_ERROR(g_logger) << "bind sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        // 没有超时时间
        if (timeout_ms == (uint64_t)-1)
        {
            // 0 success  连接需要用到address IP 所以Address一个类 socket一个类
            if (::connect(m_sock, addr->getAddr(), addr->getAddrLen()))
            {
                SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                                          << ") error errno=" << errno << " errstr=" << strerror(errno);
                close();
                return false;
            }
        }
        else
        {
            if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms))
            {
                SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                                          << ") error errno=" << errno << " errstr=" << strerror(errno);
                close();
                return false;                
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }

    bool Socket::listen(int backlog)
    {
        if (!isValid())
        {
            SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
            return false;
        }
        if(::listen(m_sock, backlog)) {
            SYLAR_LOG_ERROR(g_logger) << "listen error errno=" << errno
                << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::close()
    {
        if(!m_isConnected && m_sock == -1)
        {
            return true;
        }
        m_isConnected = false;
        if (m_sock != -1)
        {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }

    int Socket::send(const void* buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            return ::send(m_sock, buffer, length, flags);
        }
        return -1;
    }

    int Socket::send(const iovec* buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            return ::sendmsg(m_sock,&msg, flags);
        }
        return -1;
    }

    int Socket::sendTo(const void* buffers, size_t length, const Address::ptr to, int flags)
    {
        if(isConnected())
        {
            return ::sendto(m_sock, buffers, length, flags, to->getAddr(), to->getAddrLen());
        }
        return -1;
    }

    int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = to->getAddr();
            msg.msg_namelen = to->getAddrLen();
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::recv(void* buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }

    int Socket::recv(iovec* buffer, size_t length, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;        
    }

    int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags)
    {
        if (isConnected())
        {
            socklen_t len = from->getAddrLen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
        }
        return -1;
    }
    int Socket::recvFrom(iovec* buffer, size_t length, Address::ptr from, int flags)
    {
        if (isConnected())
        {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            msg.msg_name = from->getAddr();
            msg.msg_namelen = from->getAddrLen();
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1; 
    }
    
    // 得到远端的ip
    Address::ptr Socket::getRemoteAddress()
    {
        // 看远端addr是否存在
        if(m_remoteAddress) {
            return m_remoteAddress;
        }
        // 不存在就设置result，用于返回远端addr
        Address::ptr result;
        // 判断格式
        switch(m_family) {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
                break;
            default:
                result.reset(new UnknowAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrLen();
        // 用于获取与某个套接字关联的远端协议地址--存储与result->m_addr
        // on success zero return
        if(getpeername(m_sock, result->getAddr(), &addrlen)) {
            //SYLAR_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
            //    << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknowAddress(m_family));
        }
        if(m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;        
    }
    // 得到本地的ip
    Address::ptr Socket::getLocalAddress()
    {
        if(m_localAddress) {
            return m_localAddress;
        }

        Address::ptr result;
        switch(m_family) {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
                break;
            default:
                result.reset(new UnknowAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrLen();
        // 用于获取与某个套接字关联的 本地协议地址--存储与result->m_addr
        if(getsockname(m_sock, result->getAddr(), &addrlen)) {
            SYLAR_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::ptr(new UnknowAddress(m_family));
        }
        if(m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    bool Socket::isValid() const
    {
        // 不等于-1， 那就是true；
        return m_sock != -1;
    }

    int Socket::getError()
    {
        int error = 0;
        size_t len = sizeof(error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }
        return error;
    }

    std::ostream& Socket::dump(std::ostream& os) const
    {
        os << "[Socket sock=" << m_sock
            << " is_connected=" << m_isConnected
            << " family=" << m_family
            << " type=" << m_type
            << " protocol=" << m_protocol;
        if(m_localAddress) {
            os << " local_address=" << m_localAddress->toString();
        }
        if(m_remoteAddress) {
            os << " remote_address=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    std::string Socket::toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    bool Socket::cancelRead()
    {
        return IOManager::GetThis()->cancelEvent(m_sock, sylar::IOManager::READ);
    }
    bool Socket::cancelWrite()
    {
        return IOManager::GetThis()->cancelEvent(m_sock, sylar::IOManager::WRITE);
    }
    bool Socket::cancelAccept()
    {
        return IOManager::GetThis()->cancelEvent(m_sock, sylar::IOManager::READ);
    }
    bool Socket::cancelAll()
    {
        return IOManager::GetThis()->cancelAll(m_sock);
    }
    // 设置option
    void Socket::initSock()
    {
        int val = 1;
        /*
            SOL_SOCKET: 基本套接口
            IPPROTO_IP: IPv4套接口
            IPPROTO_IPV6: IPv6套接口
            IPPROTO_TCP: TCP套接口
        */

        /*
            SO_REUSERADDR 允许重用本地地址和端口 int
            充许绑定已被使用的地址（或端口号），可以参考bind的man
        */
        setOption(SOL_SOCKET, SO_REUSEADDR, val);
        // 流式套接字 TCP
        if (m_type == SOCK_STREAM)
        {
            setOption(IPPROTO_TCP, TCP_NODELAY, val);
        }
    }
    void Socket::newSock()
    {
        m_sock = socket(m_family, m_type, m_protocol);
        // SYLAR_LIKELY 就是期望 m_sock != -1成立，编译器会将if内部的代码高概率执行
        if (SYLAR_LIKELY(m_sock != -1))
        {
            initSock();
        }
        else 
        {
            SYLAR_LOG_ERROR(g_logger) << "socket(" << m_family
                << ", " << m_type << ", " << m_protocol << ") errno="
                << errno << " errstr=" << strerror(errno);
        }
    }
}