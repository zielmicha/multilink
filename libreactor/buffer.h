#ifndef BUFFER_H_
#define BUFFER_H_
#include <cstdint>
#include <string>
#include <sstream>
#include <cassert>

using size_t = std::size_t;

class Buffer {
public:
    Buffer(char* data, size_t size): size(size), data(data) {}

    size_t size;
    char* data;

    Buffer slice(size_t start, size_t size);
    Buffer slice(size_t start);
    void copy_to(Buffer target) const;
    void delete_start(size_t end);

    std::string human_repr() const;

    static const Buffer from_cstr(const char* s);

    operator std::string() {
        return std::string(data, size);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Buffer& buff) {
        return stream << "Buffer(" << buff.size << ", \"" << buff.human_repr() << "\")";
    }

    template <class T>
    T& convert(size_t pos) {
        assert(pos + sizeof(T) <= size);
        // TODO: probably violates strict aliasing
        return *((T*)(data + pos));
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
    size_t size;
    char* data;
public:
    AllocBuffer(size_t size);
    ~AllocBuffer();
    AllocBuffer(const AllocBuffer&) = delete;

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
