#include "http.h"
#include "log.h"

void test_request()
{
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello sylar");
    req->dump(std::cout) << std::endl;

}

void test_response()
{
    sylar::http::HttpResponse::ptr res(new sylar::http::HttpResponse);
    res->setHeader("X-X", "sylar");
    res->setBody("hello sylar");
    res->setStatus((sylar::http::HttpStatus)400);
    res->setClose(false);
    res->dump(std::cout) << std::endl;

}

int main(int argc, char** argv)
{
    test_request();
    test_response();
    return 0;
}