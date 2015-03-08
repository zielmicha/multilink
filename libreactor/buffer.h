#ifndef BUFFER_H_
#define BUFFER_H_
#include <cstdint>
#include <string>
#include <sstream>
#include <cassert>
#include <memory.h>

using size_t = std::size_t;

template <class F, class T>
class typealias {
    F* data;
public:
    typealias(F* data): data(data) {}

    operator T() {
        T ret;
        memcpy(&ret, data, sizeof(T));
        return ret;
    }

    typealias<F, T>& operator=(const T& val) {
        memcpy(data, &val, sizeof(T));
        return *this;
    }
};

class Buffer {
public:
    Buffer(char* data, size_t size): size(size), data(data) {}

    size_t size;
    char* data;

    Buffer slice(size_t start, size_t size) {
        assert(size >= 0 && start >= 0);
        assert(start + size <= this->size);

        return Buffer(data + start, size);
    }

    Buffer slice(size_t start) {
        return slice(start, size - start);
    }

    void copy_to(Buffer target) const {
        assert(size <= target.size);
        memcpy(target.data, data, size);
    }

    void set_zero() {
        memset(data, 0, size);
    }

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
    typealias<char, T> convert(size_t pos) {
        assert(pos + sizeof(T) <= size);
        // TODO: probably violates strict aliasing
        return typealias<char, T>(data + pos);
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
    AllocBuffer(AllocBuffer&&);
    AllocBuffer(const AllocBuffer&) = delete;

    Buffer as_buffer() {
        return Buffer(data, size);
    }
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
