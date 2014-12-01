#ifndef BUFFER_H_
#define BUFFER_H_
#include <cstdint>
#include <string>

using size_t = std::size_t;

class Buffer {
public:
    Buffer(char* data, size_t size): size(size), data(data) {}

    size_t size;
    char* data;

    Buffer slice(size_t start, size_t size);
    void copy_to(Buffer target) const;

    operator std::string() {
        return std::string(data, size);
    }
};

template <int SIZE>
class StackBuffer {
    char buff[SIZE];
public:
    operator Buffer() {
        return Buffer(buff, SIZE);
    };
};

class AllocBuffer {
    char* data;
    size_t size;
public:
    AllocBuffer(size_t size);
    ~AllocBuffer();

    Buffer as_buffer();
};

class BufferList {
    BufferList* rest;
public:
    BufferList(Buffer front);
    BufferList(Buffer front, BufferList* rest);

    Buffer front;

    bool has_next();
    BufferList& next();
    const BufferList& next() const;
};

#endif
