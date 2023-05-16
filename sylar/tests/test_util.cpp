#include "sylar.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert()
{
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    // SYLAR_ASSERT(false);
    SYLAR_ASSERT2(1 == 2, "ac");
}

int main(int argc, char** argv)
{
    // 如果是assert(0),将会把 相对应文件中代码的行号打印出来
    // assert(0);
    test_assert();
    return 0;
}