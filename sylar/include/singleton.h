#pragma once
#ifndef __SYLAR_SINGLETON_H_
#define __SYLAR_SINGLETON_H_
#include <memory>


/*  单例模式 -- 只提供唯一一个类的实例，具有全局变量的特点 
    同时禁止用户自己声明并定义实例（把构造函数设为 private）
    线程安全
    禁止赋值和拷贝
    用户通过接口获取实例：使用 static 类成员函数
*/

namespace sylar
{   
    // 范型 编程  --- 只需要传入一个class即可
    template<class T, class X = void, int N = 0>
    class Singleton
    {
        public:
            static T* GetInstance()
            {
                // 通过无参数的默认构造函数产生对象
                static T v;
                // 返回该对象地址，让指针接受该地址
                return &v;
            }
    };
    
    template<class T, class X = void, int N = 0>
    class SingletonPtr
    {
        public:
            static std::shared_ptr<T> GetInstance()
            {
                static std::shared_ptr<T> v(new T);
                return v;
            }
    };
}

#endif