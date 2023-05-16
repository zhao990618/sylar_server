#include "sylar.h"
#include "iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <iostream>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int fd = 0;

void testFiber()
{
    SYLAR_LOG_INFO(g_logger) << "test_fiber fd=" << fd;
    // 创建一个socket  --- 0，1，2为系统默认，3是m_epfd,4,5是读端和写端, fd = 6
    fd = socket(AF_INET, SOCK_STREAM, 0);
    // SYLAR_ASSERT(fd != -1);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;   // ipv4
    addr.sin_port = htons(8080); // 端口
    inet_pton(AF_INET, "14.215.177.38", &addr.sin_addr.s_addr);
    //如果请求连接成功，则返回0，否则返回错误码。
    if (!connect(fd, (const sockaddr *)&addr, sizeof(addr)))
    {
    }
    else if (errno == EINPROGRESS)
    {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        // sylar::IOManager::GetThis()->addEvent(fd, sylar::IOManager::READ, []()
        //                                       { SYLAR_LOG_INFO(g_logger) << "read callback"; });
        SYLAR_LOG_INFO(g_logger) << "=====================";
        sylar::IOManager::GetThis()->addEvent(fd, sylar::IOManager::WRITE, []()
                                              { 
                                                SYLAR_LOG_INFO(g_logger) << "write callback"; 
                                                sylar::IOManager::GetThis()->cancelEvent(fd, sylar::IOManager::READ);
                                                close(fd); });
        SYLAR_LOG_INFO(g_logger) << "add event end";
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "else" << errno << " " << strerror(errno);
    }
}

void test_fiber_1()
{
    SYLAR_LOG_INFO(g_logger) << "test fiber 1";
}

void test1()
{
    std::cout << " EPOLLIN =" << EPOLLIN
              << " EPOLLOUT =" << EPOLLOUT << std::endl;
    sylar::IOManager iom;
    SYLAR_LOG_INFO(g_logger) << "test1--> iom -- schedule";
    // iom.schedule(&test_fiber_1);
    iom.schedule(&testFiber);

}

sylar::Timer::ptr s_timer;
void test_timer()
{
    sylar::IOManager iom(2, true, "main");
    s_timer = iom.addTimer(
        500, []()
        {
            static int i = 0;
            SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
            
            if (++i == 3)
            {
                s_timer->reset(2000, true);
                // s_timer->cancel();
            } 
        },
        true);
}

int main(int argc, char **argv)
{
    // test1();
    test_timer(); // 40:02
    return 0;
}