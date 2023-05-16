#pragma once
/* 获得 线程 协程 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <vector>
#include <string>


#ifndef __SYLAR_UTIL_H_
#define __SYLAR_UTTL_H_
namespace sylar
{
    pid_t GetThreadId();
    uint32_t GetFiberId();
    void Backtrace(std::vector<std::string>& vec, int size = 64, int skip = 1);
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");
    // 时间 毫秒 ms
    uint64_t GetCurrentMS();
    // 时间 微秒 us
    uint64_t GetCurrentUS();
}   

#endif