#include "tcp_server.h"
#include "iomanager.h"
#include "log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run()
{
    auto addr = sylar::Address::LookupAny("0.0.0.0:8033");
    // auto addr1 = sylar::UnixAddress::ptr(new sylar::UnixAddress("/home/zhaoyangfan/LinuxStudio/server/sylar/unix_addr"));
    // SYLAR_LOG_INFO(g_logger) << *addr1;
    std::vector<sylar::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr1);
    sylar::TcpServer::ptr tcp_server(new sylar::TcpServer);
    std::vector<sylar::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails))
    {
        sleep(2);
    }
    SYLAR_LOG_INFO(g_logger) << "will start";
    tcp_server->start();
    // std::vector<sylar::Address::ptr> addrs;
    // std::vector<sylar::Address::ptr> fails;
    // addrs.push_back(addr);
    
    // sylar::TcpServer::ptr tcp_server(new sylar::TcpServer);
    
    // while(!tcp_server->bind(addrs, fails))
    // {
    //     sleep(2);
    // }
    // tcp_server->start();
}

int main(int argc, char** argv)
{

    sylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}