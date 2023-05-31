#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>

namespace sylar
{
    class ByteArray
    {
        public:
            typedef std::shared_ptr<ByteArray> ptr;

            struct Node
            {
                Node(size_t s);
                Node();
                ~Node();

                char* ptr;
                Node* next;
                size_t size;
            };

            // 每一块链表的长度为4K
            ByteArray(size_t base_size = 4096);
            ~ByteArray();

            // write  1个字节到8个字节的int都提供
            void writeFint8  (const int8_t& value);
            void writeFuint8 (const uint8_t& value);
            void writeFint16 (const int16_t& value);
            void WriteFuint16(const uint16_t& value);
            void writeFint32 (const int32_t& value);
            void writeFuint32(const uint32_t& value);
            void writeFint64 (const int64_t& value);
            void writeFuint64(const uint64_t& value);

            // 可变长度
            void writeInt32  (const int32_t& value);
            void writeUint32 (const uint32_t& value);
            void writeInt64  (const int64_t& value);
            void writeUint64 (const uint64_t& value);

            void writeFloat  (const float& value);
            void writeDouble (const double& value);
            // length: int16 data
            void writeStringF16 (const std::string& value);
            // length: int32 data
            void writeStringF32 (const std::string& value);
            // length: int64 data
            void writeStringF64 (const std::string& value);
            // length: varint data
            void writeStringVint(const std::string& value);
            // data
            void writeStringWithoutLength(const std::string& value);

            // read
            int8_t readFint8  ();
            uint8_t readFuint8 ();
            int16_t readFint16 ();
            uint16_t readFuint16();
            int32_t readFint32 ();
            uint32_t readFuint32();
            int64_t readFint64 ();
            uint64_t readFuint64();

             // 可变长度
            int32_t readInt32  ();
            uint32_t readUint32 ();
            int64_t readInt64  ();
            uint64_t readUint64 ();           

            float readFloat  ();
            double readDouble ();

            // length: int16 data
            std::string& readStringF16 ();
            // length: int32 data
            std::string& readStringF32 ();
            // length: int64 data
            std::string& readStringF64 ();
            // length: varint data
            std::string& readStringVint();

            // 内部操作
            void clear();
            void write(const void* buf, size_t size);
            void read(char* buf, size_t size);

            size_t getPosition() const {return m_position;};
            void setPosition(size_t v);

            bool writeToFile(const std::string& name) const;
            void readFromFile(const std::string& name);

            // 返回Node的大小
            size_t getBaseSize() const {return m_baseSize;}
            // 返回还可以读的大小
            size_t getReadSize() const {return m_size - m_position;}

            // 设置网络字节序
            bool isLittleEndian() const;
            void setIsLittleEndian(bool val);

        private:
            // 设置当前的容量
            void setCapacity(size_t size);
            // 返回当前剩余的容量
            size_t getCapacity() const {return m_capacity - m_position;}
        private:
            // 每一个Node的大小
            size_t m_baseSize;
            // 当前操作的位置
            size_t m_position;
            // 总容量 
            size_t m_capacity;           
            // 当前的真实大小
            size_t m_size;
            // 链表的根节点指针
            Node* m_root;
            // 链表的当前操作指针
            Node* m_cur;
            // 网络字节序是大端传输
            int8_t m_endian;
    };
}

#endif