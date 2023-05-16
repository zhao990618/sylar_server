#include "util.h"
#include <execinfo.h>
#include "log.h"
#include "fiber.h"
#include <sys/time.h>

namespace sylar
{

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId()
    {
        return sylar::Fiber::GetFiberId();
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        // void* 大小 * size, array是一个数组指针，保存的都是指针，总共有size个指针
        void **array = (void **)malloc((sizeof(void *) * size));

        // ::backtrace 表示的是搜索外层作用域中的backtrace函数，因为在当前命名空间中也
        // 会存在 backtrace函数。如果同名，则最近原则就会调用本命名空间内的backteace
        size_t s = ::backtrace(array, size);

        // 将栈中的内容全都压入到strings这个指针数组中
        char **strings = backtrace_symbols(array, s);

        if (NULL == strings)
        {
            SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
            free(strings);
            free(array);
            return;
        }

        // 开头等于skip，相当于跳过skip个信息
        for (size_t i = skip; i < s; ++i)
        {
            // 将strings中的信息填入 vector中
            bt.push_back(strings[i]);
        }
        // 释放指针
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i)
        {
            // prefix 可以是换行什么的
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    // 时间 毫秒 ms
    uint64_t GetCurrentMS()
    {
        // 时间变量
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;

    }
    // 时间 微秒 us
    uint64_t GetCurrentUS()
    {
        // 时间变量
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }

};