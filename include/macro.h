#pragma once
#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__
#include <string>
#include <assert.h>
#include "util.h"

/*
    __builtin_expect (exp, c) ---> exp（返回值为exp）
    期望 exp 表达式的值等于常量 c, 看 c 的值, 
    如果 c 的值为0(即期望的函数返回值), 那么 执行 if 分支的的可能性小,
    如果为1，则执行if分支的可能性很大

    因为期望是0，所以exp表达式的值被期望为0，编译器为了优化直接省略if内部，直接执行else
    if (__builtin_expect(x, 0))
    {
        func();
    }else{　
    　//do someting
    }
*/

// 如果是一下两个编译器
#if defined __GNUC__ || defined __llvm__
//// LICKLY 宏的封装， 告诉编译器优化，条件大概率成立
#       define SYLAR_LIKELY(x)      __builtin_expect(!!(x), 1)
//// LILCKY 宏的封装, 告诉编译器优化,条件大概率不成立
#       define SYLAR_UNLIKELY(x)      __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)      (x)
#endif


// 断言 是很小概率触发的，所以加上UNLICKLY
#define SYLAR_ASSERT(x)                                                                                                                                                   \
    if (SYLAR_UNLIKELY(!(x)))                                                                          \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\nbacktrace\n"                           \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

#define SYLAR_ASSERT2(x, w)                                                                                                                                                 \
    if (SYLAR_UNLIKELY(!(x)))                                                                          \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\n" << w                              \
                                          << "\nbacktrace\n"                           \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }
#endif