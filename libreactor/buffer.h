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

#endif
