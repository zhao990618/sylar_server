#include "tcp_server.h"
#include "log.h"
#include "config.h"
namespace sylar
{

    static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    TcpServer::TcpServer(sylar::IOManager* worker
                        ,sylar::IOManager* io_worker
                        ,sylar::IOManager* accept_worker)
        :m_worker(worker)
        ,m_ioWorker(io_worker)
        ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
        ,m_acceptWorker(accept_worker)
        ,m_name("sylar/1.0.0")
        ,m_isStop(true)
    {

    }

    TcpServer::~TcpServer()
    {
        for(auto& i : m_socks)
        {
            i->close();
        }
        m_socks.clear();
    }

    // 一个地址的bind
    bool TcpServer::bind(sylar::Address::ptr addr, bool ssl)
    {
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails;
        addrs.push_back(addr);
        return bind(addrs, fails, ssl);
    }
    // 一堆地址的bind
    bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails
                        ,bool ssl)
    {
        m_ssl = ssl;
        for(auto& addr : addrs) {
            // Socket::ptr sock = ssl ? SSLSocket::CreateTCP(addr) : Socket::CreateTCP(addr);
            Socket::ptr sock = Socket::CreateTCP(addr);
            if(!sock->bind(addr)) {
                SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                    << errno << " errstr=" << strerror(errno)
                    << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            if(!sock->listen()) {
                SYLAR_LOG_ERROR(g_logger) << "listen fail errno="
                    << errno << " errstr=" << strerror(errno)
                    << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }
        // 失败的
        if(!fails.empty()) {
            m_socks.clear();
            return false;
        }
        // 成功的地址取出来
        for(auto& i : m_socks) {
            SYLAR_LOG_INFO(g_logger) << "type=" << m_type
                << " name=" << m_name
                << " ssl=" << m_ssl
                << " server bind success: " << *i;
        }
        return true;
    }

    void TcpServer::startAccept(Socket::ptr sock) {
        while(!m_isStop) {
            Socket::ptr client = sock->accept();
            SYLAR_LOG_INFO(g_logger) << client;
            if(client) {
                client->setRecvTimeout(m_recvTimeout);
                m_ioWorker->schedule(std::bind(&TcpServer::handleClient,
                            shared_from_this(), client));
            } else {
                SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                    << " errstr=" << strerror(errno);
            }
        }
    }

    // 服务器的启动
    bool TcpServer::start()
    {
        if(!m_isStop) {
            return true;
        }
        m_isStop = false;
        for(auto& sock : m_socks) {
            // bind(funName, point, 参数)
            m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                        shared_from_this(), sock));
        }
        return true;
    }
    // 服务器的停止
    void TcpServer::stop()
    {
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptWorker->schedule([this, self]() {
            for(auto& sock : m_socks) {
                sock->cancelAll();
                sock->close();
            }
            m_socks.clear();
        });
    }

    void TcpServer::handleClient(Socket::ptr client) {
        SYLAR_LOG_INFO(g_logger) << "handleClient: " << *client;
    }
}