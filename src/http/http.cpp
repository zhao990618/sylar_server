#include <http.h>

namespace sylar
{
    namespace http
    {
        // 将字符串方法名转成HTTP方法枚举
        HttpMethod StringToHttpMethod(const std::string &m)
        {
/*
若s1、s2字符串相等，则返回零；
若s1大于s2，则返回大于零的数；否则，则返回小于零的数

    如果string& m 能够和 #string匹配的上，那就返回name值
    否则就是INVALID_METHOD
*/
#define XX(num, name, string)            \
    if (strcmp(#string, m.c_str()) == 0) \
    {                                    \
        return HttpMethod::name;         \
    }                                    \
    HTTP_METHOD_MAP(XX);
#undef XX
            return HttpMethod::INVALID_METHOD;
        }

        // 将字符串指针转换成HTTP方法枚举
        HttpMethod CharsToHttpMethod(const char *m)
        {
            /*
            C 库函数 int strncmp(const char *str1, const char *str2, size_t n)
            把 str1 和 str2 进行比较，最多比较前 n 个字节
            */

#define XX(num, name, string)                      \
    if (strncmp(#string, m, strlen(#string)) == 0) \
    {                                              \
        return HttpMethod::name;                   \
    }
            HTTP_METHOD_MAP(XX);
#undef XX
            return HttpMethod::INVALID_METHOD;
        }

        // 将所有的http方法的字符串表示都 存放在 s_method_string
        static const char *s_method_string[] = {
#define XX(num, name, string) #string,
            HTTP_METHOD_MAP(XX)
#undef XX
        };

        // 将HTTP方法枚举转换成字符串
        const char *HttpMethodToString(const HttpMethod &m)
        {
            uint32_t idx = (uint32_t)m;
            // 如果idx越界了，超出了存储范围
            if (idx >= (sizeof(s_method_string) / sizeof(s_method_string[0])))
            {
                return "<unknown>";
            }
            return s_method_string[idx];
        }

        // 将HTTP状态枚举转换成字符串
        const char *HttpStatusToString(const HttpStatus &s)
        {
            switch (s)
            { /*
              name ---> code  ----> s
              */
#define XX(code, name, msg) \
    case HttpStatus::name:  \
        return #msg;
                HTTP_STATUS_MAP(XX);
#undef XX
            default:
                return "<unknown>";
            }
        }

        bool CaseInsensitiveLess::operator()(const std::string &lhs, const std::string &rhs) const
        {
            // 不区分大小写 的 对比
            return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
        }

        HttpRequest::HttpRequest(uint8_t version, bool close)
            : m_method(HttpMethod::GET), m_version(version), m_close(close), m_websocket(false), m_parserParamFlag(0), m_path("/")
        {
        }

        std::string HttpRequest::getHeaders(const std::string &key, const std::string &def) const
        {
            auto it = m_headers.find(key);
            return it == m_headers.end() ? def : it->second;
        }

        std::string HttpRequest::getParams(const std::string &key, const std::string &def) const
        {
            auto it = m_params.find(key);
            return it == m_params.end() ? def : it->second;
        }

        std::string HttpRequest::getCookies(const std::string &key, const std::string &def) const
        {
            auto it = m_cookies.find(key);
            return it == m_cookies.end() ? def : it->second;
        }

        void HttpRequest::setHeader(const std::string &key, const std::string &val)
        {
            m_headers[key] = val;
        }

        void HttpRequest::setParams(const std::string &key, const std::string &val)
        {
            m_params[key] = val;
        }

        void HttpRequest::setCookies(const std::string &key, const std::string &val)
        {
            m_cookies[key] = val;
        }

        void HttpRequest::delHeader(const std::string &key)
        {
            m_headers.erase(key);
        }

        void HttpRequest::delParams(const std::string &key)
        {
            m_params.erase(key);
        }

        void HttpRequest::delCookies(const std::string &key)
        {
            m_cookies.erase(key);
        }

        // 获得 相应的header
        bool HttpRequest::hasHeader(const std::string &key, std::string *val)
        {
            auto it = m_headers.find(key);
            if (it == m_headers.end())
            {
                return false;
            }
            if (val)
            {
                val = &(it->second);
            }
            return true;
        }

        bool HttpRequest::hasParams(const std::string &key, std::string *val)
        {
            auto it = m_params.find(key);
            if (it == m_params.end())
            {
                return false;
            }
            if (val)
            {
                val = &(it->second);
            }
            return true;
        }

        bool HttpRequest::hasCookies(const std::string &key, std::string *val)
        {
            auto it = m_cookies.find(key);
            if (it == m_cookies.end())
            {
                return false;
            }
            if (val)
            {
                val = &(it->second);
            }
            return true;
        }

        std::string HttpRequest::toString() const
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }

        std::ostream &HttpRequest::dump(std::ostream &os) const
        {
            // GET /uri HTTP/1.1
            // Host: www.sylar.top
            //
            //
            /*
                uri 部分
            */
            os << HttpMethodToString(m_method) << " "
               << m_path
               << (m_query.empty() ? "" : "?")
               << m_query
               << (m_fragment.empty() ? "" : "#")
               << m_fragment
               << " HTTP/"
               << ((uint32_t)(m_version >> 4))
               << "."
               << ((uint32_t)(m_version & 0x0F))
               << "\r\n";

            if (!m_websocket)
            {
                os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
            }

            for (auto &i : m_headers)
            { // strcasecmp -- C语言中判断字符串是否相等的函数，忽略大小写
                if (!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0)
                {
                    continue;
                }
                os << i.first << ": " << i.second << "\r\n";
            }

            // 如果body不是为空，那就要输出长度
            if (!m_body.empty())
            {
                os << "content-length: " << m_body.size() << "\r\n\r\n"
                   << m_body;
            }
            else
            {
                os << "\r\n";
            }
            return os;
        }

        void HttpRequest::init()
        {
            std::string conn = getHeaders("connection");
            if (!conn.empty())
            {
                if (strcasecmp(conn.c_str(), "keep-alive") == 0)
                {
                    m_close = false;
                }
                else
                {
                    m_close = true;
                }
            }
        }

        HttpResponse::HttpResponse(uint8_t version, bool close)
            : m_status(HttpStatus::OK), m_version(version), m_close(close)
        {
        }

        std::string HttpResponse::getHeader(const std::string &key, const std::string &def) const
        {
            auto it = m_headers.find(key);
            return (it == m_headers.end() ? def : it->second);
        }

        void HttpResponse::setHeader(const std::string &key, const std::string &val)
        {
            m_headers[key] = val;
        }

        void HttpResponse::delHeader(const std::string &key)
        {
            m_headers.erase(key);
        }

        std::string HttpResponse::toString() const
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }

        std::ostream &HttpResponse::dump(std::ostream &os) const
        {
            os << "HTTP/"
               << ((uint32_t)(m_version >> 4))
               << "."
               << ((uint32_t)(m_version & 0x0F))
               << " "
               << (uint32_t)m_status
               << " "
               << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
               << "\r\n";

            for (auto &i : m_headers)
            {
                if (!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0)
                {
                    continue;
                }
                os << i.first << ": " << i.second << "\r\n";
            }
            for (auto &i : m_cookies)
            {
                os << "Set-Cookie: " << i << "\r\n";
            }
            if (!m_websocket)
            {
                os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
            }
            if (!m_body.empty())
            {
                os << "content-length: " << m_body.size() << "\r\n\r\n"
                   << m_body;
            }
            else
            {
                os << "\r\n";
            }
            return os;
        }
        
        std::ostream& operator<<(std::ostream& os, const HttpRequest& req)
        {
            return req.dump(os);
        }

        std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp)
        {
            return rsp.dump(os);
        }

    }
}