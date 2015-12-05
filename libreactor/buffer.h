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

    const Buffer slice(size_t start) const {
        return ((Buffer*)this)->slice(start);
    }

    const Buffer slice(size_t start, size_t size) const {
        return ((Buffer*)this)->slice(start, size);
    }

    void copy_to(Buffer target) const {
        assert(size <= target.size);
        memcpy(target.data, data, size);
    }

    void copy_from(const Buffer from) {
        from.copy_to(*this);
    }

    void set_zero() {
        memset(data, 0, size);
    }

    void delete_start(size_t end, size_t actual_size);
    void delete_start(size_t end) {
        delete_start(end, size);
    }

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
public:
    size_t size;
    char* data;

    AllocBuffer(size_t size);
    AllocBuffer();
    ~AllocBuffer();
    AllocBuffer(AllocBuffer&&);
    AllocBuffer(const AllocBuffer&) = delete;

    AllocBuffer& operator=(AllocBuffer&&);
    AllocBuffer& operator=(const AllocBuffer&) = delete;

    static AllocBuffer copy(Buffer);
    bool empty() { return data == nullptr; }

    Buffer as_buffer() {
        return Buffer(data, size);
    }
};

template <typename T>
class CastAsBuffer {
public:
    Buffer cast(T& t) {
        return t.as_buffer();
    }
    typedef Buffer Target;
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
