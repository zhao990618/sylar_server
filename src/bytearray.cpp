#include "bytearray.h"
#include "sendian.h"
#include <string.h>
#include <iomanip>
#include "log.h"
#include <math.h>
#include <sstream>
namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
    ByteArray::Node::Node(size_t s)
        : ptr(new char[s]),  next(nullptr), size(s)
    {
    }
    ByteArray::Node::Node()
        : ptr(nullptr), next(nullptr), size(0)
    {
    }
    ByteArray::Node::~Node()
    {
        if (ptr)
        {
            delete[] ptr;
        }
    }

    ByteArray::ByteArray(size_t base_size)
        : m_baseSize(base_size), m_position(0), m_capacity(base_size), m_size(0), m_root(new Node(base_size)), m_cur(m_root), m_endian(SYLAR_BIG_ENDIAN)
    {
    }
    ByteArray::~ByteArray()
    {
        Node *tmp = m_root;
        while (tmp)
        {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
    }

    bool ByteArray::isLittleEndian() const
    {
        return m_endian == SYLAR_LITTLE_ENDIAN;
    }

    void ByteArray::setIsLittleEndian(bool val)
    {
        if (val)
        {
            m_endian = SYLAR_LITTLE_ENDIAN;
        }
        else
        {
            m_endian = SYLAR_BIG_ENDIAN;
        }
    }
    // write  1个字节到8个字节的int都提供
    void ByteArray::writeFint8(int8_t value)
    {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFuint8(uint8_t value)
    {
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint16(int16_t value)
    {
        // 如果 字节序不相同
        if (m_endian != SYLAR_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint16(uint16_t value)
    {
        if (m_endian != SYLAR_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint32(int32_t value)
    {
        if (m_endian != SYLAR_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint32(uint32_t value)
    {
        if (m_endian != SYLAR_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFint64(int64_t value)
    {
        if (m_endian != SYLAR_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }
    void ByteArray::writeFuint64(uint64_t value)
    {
        if (m_endian != SYLAR_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    // 编码
    static uint32_t EncodeZigzag32(const int32_t &v)
    {
        if (v < 0)
        {
            return ((uint32_t)(-v)) * 2 - 1;
        }
        else
        {
            return v * 2;
        }
    }

    static uint64_t EncodeZigzag64(const int64_t &v)
    {
        if (v < 0)
        {
            return ((uint64_t)(-v)) * 2 - 1;
        }
        else
        {
            return v * 2;
        }
    }

    // 解码
    static int32_t DecodeZigzag32(const uint32_t &v)
    {
        return (v >> 1) ^ -(v & 1);
    }

    static int64_t DecodeZigzag64(const uint64_t &v)
    {
        return (v >> 1) ^ -(v & 1);
    }

    // 可变长度
    void ByteArray::writeInt32(int32_t value)
    {
        writeUint32(EncodeZigzag32(value));
    }
    void ByteArray::writeUint32(uint32_t value)
    {
        // 7bit压缩算法，8个bit位，第一位如果是1的话说明还有值
        uint8_t tmp[5];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeInt64(int64_t value)
    {
        writeUint64(EncodeZigzag64(value));
    }

    void ByteArray::writeUint64(uint64_t value)
    {
        uint8_t tmp[10];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeFloat(float value)
    {
        uint32_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint32(v);
    }

    void ByteArray::writeDouble(double value)
    {
        uint64_t v;
        memcpy(&v, &value, sizeof(value));
        writeFuint64(v);
    }
    // length: int16 data
    void ByteArray::writeStringF16(const std::string &value)
    {
        writeFuint16(value.size());
        // 第一个字符的地址
        write(value.c_str(), value.size());
    }
    // length: int32 data
    void ByteArray::writeStringF32(const std::string &value)
    {
        writeFuint32(value.size());
        // 第一个字符的地址
        write(value.c_str(), value.size());
    }
    // length: int64 data
    void ByteArray::writeStringF64(const std::string &value)
    {
        writeFuint64(value.size());
        // 第一个字符的地址
        write(value.c_str(), value.size());
    }
    // length: varint data
    void ByteArray::writeStringVint(const std::string &value)
    {
        writeUint64(value.size());
        write(value.c_str(), value.size());
    }
    // data --- 不需要长度
    void ByteArray::writeStringWithoutLength(const std::string &value)
    {
        write(value.c_str(), value.size());
    }

    // read
    int8_t ByteArray::readFint8()
    {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }
    uint8_t ByteArray::readFuint8()
    {
        uint8_t v;
        read(&v, sizeof(v));
        return v;
    }

// 用宏来实现
#define XX(type)                      \
    type v;                           \
    read(&v, sizeof(v));              \
    if (m_endian == SYLAR_BYTE_ORDER) \
    {                                 \
        return v;                     \
    }                                 \
    else                              \
    {                                 \
        return byteswap(v);           \
    }

    int16_t ByteArray::readFint16()
    {
        XX(int16_t);
    }
    uint16_t ByteArray::readFuint16()
    {
        XX(uint16_t);
    }
    int32_t ByteArray::readFint32()
    {
        XX(int32_t);
    }
    uint32_t ByteArray::readFuint32()
    {
        XX(uint32_t);
    }
    int64_t ByteArray::readFint64()
    {
        XX(int64_t);
    }
    uint64_t ByteArray::readFuint64()
    {
        XX(uint64_t);
    }

#undef XX

    // 可变长度
    int32_t ByteArray::readInt32()
    {
        return DecodeZigzag32(readUint32());
    }

    uint32_t ByteArray::readUint32()
    {
        uint32_t result = 0;
        // 压缩时是7位压缩，所以解压时需要7位解压
        for (int i = 0; i < 32; i += 7)
        {
            uint8_t b = readFuint8();
            // 如果小于0x80就是没有值了
            if (b < 0x80)
            {
                result |= ((uint32_t)b) << i;
                break;
            }
            else
            { // 每7位 添加一次值
                result |= (((uint32_t)(b & 0x7F)) << i);
            }
        }
        return result;
    }

    int64_t ByteArray::readInt64()
    {
        return DecodeZigzag64(readUint64());
    }

    uint64_t ByteArray::readUint64()
    {
        uint64_t result = 0;
        // 压缩时是7位压缩，所以解压时需要7位解压
        for (int i = 0; i < 64; i += 7)
        {
            uint8_t b = readFuint8();
            // 如果小于0x80就是没有值了
            if (b < 0x80)
            {
                result |= ((uint64_t)b) << i;
                break;
            }
            else
            { // 每7位 添加一次值
                result |= (((uint64_t)(b & 0x7F)) << i);
            }
        }
        return result;
    }

    float ByteArray::readFloat()
    {
        uint32_t v = readFuint32();
        float value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }
    double ByteArray::readDouble()
    {
        uint64_t v = readFuint64();
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    // length: int16 data
    std::string ByteArray::readStringF16()
    {
        // 因为write 是先将length输出，在输出数据值
        // 所以read  要将length读取，再决定用多长大小的内存存储后来的数据
        uint16_t len = readFuint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }
    // length: int32 data
    std::string ByteArray::readStringF32()
    {
        uint32_t len = readFuint32();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }
    // length: int64 data
    std::string ByteArray::readStringF64()
    {
        uint64_t len = readFuint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }
    // length: varint data
    std::string ByteArray::readStringVint()
    {
        uint64_t len = readUint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    // 内部操作
    void ByteArray::clear()
    {
        m_position = m_size = 0;
        // 只留下一个节点,其他节点释放
        m_capacity = m_baseSize;
        Node* tmp = m_root->next;
        while(tmp)
        {
            m_cur = tmp;
            tmp = tmp->next;
            delete m_cur;
        }
        m_cur = m_root;
        m_root->next = NULL;
    }

    void ByteArray::write(const void *buf, size_t size)
    {
        if (size == 0)
        {
            return;
        }
        // 设置内存大小
        addCapacity(size);
        // 当前节点位置
        size_t npos = m_position % m_baseSize;
        // 当前节点容量
        size_t ncap = m_cur->size - npos;
        // 已经写入的偏移量
        size_t bpos = 0;

        while (size > 0)
        {
            // 如果容量大于 size，即可以完全装下
            if (ncap >= size)
            {
                memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
                if(m_cur->size == (npos + size)) {
                    m_cur = m_cur->next;
                }
                // 写完后，位置后移size位
                m_position += size;
                // 已经写入的偏移量 + size
                bpos += size;
                size = 0;
            }
            else
            {
                memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
                // 写完后，位置后移size位
                m_position += ncap;
                // 已经写入的偏移量 + size
                bpos += ncap;
                size -= ncap;
                // cur指针要往后跳， 因为当前Node已经写满了
                m_cur = m_cur->next;
                // 当前节点容量
                ncap = m_cur->size;
                // 当前新节点的偏移量为0
                bpos = 0;
            }
        }

        if (m_position > m_size)
        {
            m_size = m_position;
        }

    }

    void ByteArray::read(void *buf, size_t size)
    {
        // 如果长度超过了当前 剩余的可读长度
        if (size > getReadSize())
        {
            throw std::out_of_range("not enough len");
        }
        // 当前节点位置
        size_t npos = m_position % m_baseSize;
        // 当前节点容量
        size_t ncap = m_cur->size - npos;
        // 已经写入的偏移量
        size_t bpos = 0;
        while(size > 0)
        {
            // 容量足够放下size
            if (ncap >= size)
            {
                memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
                if (m_cur->size == (npos + size))
                {
                    m_cur = m_cur->next;
                }
                m_position += size;
                bpos += size;
                size = 0;
            } 
            else
            {
                memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
                m_position += ncap;
                bpos += ncap;
                size -= ncap;
                m_cur = m_cur->next;
                ncap = m_cur->size;
                npos = 0;
            }  
        }
    }

    // 并不会改变当前指针的位置
    void ByteArray::read(void* buf, size_t size, size_t position) const
    {
        // 如果长度超过了当前 剩余的可读长度
        if (size > (m_size - position))
        {
            throw std::out_of_range("not enough len");
        }
        // 当前节点位置
        size_t npos = position % m_baseSize;
        // 当前节点容量
        size_t ncap = m_cur->size - npos;
        // 已经写入的偏移量
        size_t bpos = 0;
        Node* cur = m_cur;
        while(size > 0)
        {
            // 容量足够放下size
            if (ncap >= size)
            {
                memcpy((char*)buf + bpos, cur->ptr + npos, size);
                if (cur->size == (npos + size))
                {
                    cur = cur->next;
                }
                position += size;
                bpos += size;
                size = 0;
            } 
            else
            {
                memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }  
        }
    }

    void ByteArray::setPosition(size_t v)
    {
        // 如果超出最大范围
        if (v > m_size)
        {
            throw std::out_of_range("set_position out of range");
        }
        // 更新position
        m_position = v;
        if (m_position > m_size)
        {
            m_size = m_position;
        }
        // 更新cur
        m_cur = m_root;
        while(v > m_cur->size)
        {
            v -= m_cur->size;
            m_cur = m_cur->next;
        }
        if (v == m_cur->size)
        {
            m_cur = m_cur->next;
        }
    }

    bool ByteArray::writeToFile(const std::string &name) const
    {
        std::ofstream ofs;
        ofs.open(name, std::ios::trunc | std::ios::binary);
        if (!ofs)
        {
            SYLAR_LOG_INFO(g_logger) << "writeToFile name=" << name 
                    << " error, errno=" << errno << "errstr=" << strerror(errno);
            return false;
        }
        // 可读大小
        int64_t read_size = getReadSize();
        int64_t pos = m_position;
        Node* cur = m_cur;
        while(read_size > 0) {
            // 
            int diff = pos % m_baseSize;
            // 可以填入的长度
            int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
            // 写入到ofs中
            ofs.write(cur->ptr + diff, len);
            cur = cur->next;
            pos += len;
            read_size -= len;
        }
        return true;
    }

    bool ByteArray::readFromFile(const std::string &name)
    {
        std::ifstream ifs;
        ifs.open(name, std::ios::binary);
        if (!ifs)
        {
            SYLAR_LOG_ERROR(g_logger) << "readFromFile name=" << name
                << " error, errno=" << errno << " errstr=" << strerror(errno);
            return false;            
        }
        // buff智能指针 --- 指向一个char类型的数组，然后自带一个析构函数[内保存的是传入的参数]（内保存的是自己创建的函数）
        std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
        // eof() 是读取到文件结束符 返回true
        while(!ifs.eof()) 
        {
            ifs.read(buff.get(), m_baseSize);
            // gcount 返回最后一个输入操作读取的字符数目。
            write(buff.get(), ifs.gcount());
        }
        return true;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
        {
            return 0;
        }
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        struct iovec iov;
        Node* cur = m_cur;

        while(len > 0) {
            if(ncap >= len) {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = len;
                len = 0;
            } else {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if(len == 0) {
            return 0;
        }

        uint64_t size = len;

        size_t npos = position % m_baseSize;
        size_t count = position / m_baseSize;
        Node* cur = m_root;
        while(count > 0) {
            cur = cur->next;
            --count;
        }

        size_t ncap = cur->size - npos;
        struct iovec iov;
        while(len > 0) {
            if(ncap >= len) {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = len;
                len = 0;
            } else {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len)
    {
        if(len == 0) {
            return 0;
        }
        addCapacity(len);
        uint64_t size = len;

        size_t npos = m_position % m_baseSize;
        size_t ncap = m_cur->size - npos;
        struct iovec iov;
        Node* cur = m_cur;
        while(len > 0) {
            if(ncap >= len) {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = len;
                len = 0;
            } else {
                iov.iov_base = cur->ptr + npos;
                iov.iov_len = ncap;

                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.push_back(iov);
        }
        return size;
    }

    void ByteArray::addCapacity(size_t size)
    {
        if (size == 0)
        {
            return;
        }
        size_t old_cap = getCapacity();
        // 剩余的容量如果比size大，那就直接退出，因为有充足的空间存储
        if (old_cap >= size)
        {
            return;
        }
        // 超了的size大小
        size = size - old_cap;
        // 需要额外添加多少个节点
        // ceil 函数用于对浮点数 float 或者 double 或者 longdouble 向上取整
        size_t count = ceil(1.0 * size / m_baseSize);
        Node* tmp = m_root;
        // 来到最末尾的node节点
        while(tmp->next)
        {
            tmp = tmp->next;
        }
        // 把添加上的第一个节点记录下来
        Node* first = NULL;
        for (size_t i = 0; i < count; ++i)
        {
            tmp->next = new Node(m_baseSize);
            if (first == NULL)
            {
                first = tmp->next;
            }
            tmp = tmp->next;
            // 每添加一个节点，总体容量就要添加一个Node(m_baseSize)大小
            m_capacity += m_baseSize;
        }
        // 如果 当前没有空缺的数据空间，那么m_cur一定是指向第一个新建的Node
        if (old_cap == 0)
        {
            m_cur = first;
        }// 否则，m_cur就是指向当前的Node 
    }

    std::string ByteArray::toString() const
    {
        std::string str;
        // 有多少可读的空间就设置多大str大小
        str.resize(getReadSize());
        if (str.empty())
        {
            return str;
        }
        // 从m_position位置开始读，并且并不会改变m_position
        read(&str[0], str.size(), m_position);
        return str;
    }

    std::string ByteArray::toHexString() const
    {
        std::string str = toString();
        std::stringstream ss;

        for (size_t i = 0; i < str.size(); i++)
        {        // 一行显示32个字符
            if (i > 0 && i % 32 == 0)
            {
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex
                               << (int)(uint8_t)str[i] << " ";
        }
        return ss.str();
    }
}