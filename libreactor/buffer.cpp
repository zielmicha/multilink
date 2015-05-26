#include "buffer.h"
#include <cassert>
#include <sstream>
#include <iomanip>

void Buffer::delete_start(size_t end, size_t actual_size) {
    assert(actual_size >= end);
    assert(size >= actual_size);
    memmove(data, data + end, actual_size - end);
}

const Buffer Buffer::from_cstr(const char* s) {
    return Buffer((char*)s, strlen(s));
}

std::string Buffer::human_repr() const {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for(size_t i=0; i < size; i ++) {
        char c = data[i];
        if((isalnum(c) || c == ' ' || ispunct(c)) && c != '\\')
            escaped << c;
        else {
            escaped << "\\x";
            escaped << std::setw(2) << int((unsigned char) c);
        }
    }
    return escaped.str();
}

AllocBuffer::AllocBuffer(size_t size): size(size) {
    data = new char[size];
}

AllocBuffer::AllocBuffer(): size(0), data(nullptr) {}

AllocBuffer::~AllocBuffer() {
    delete[] data;
}

AllocBuffer::AllocBuffer(AllocBuffer&& other): size(other.size), data(other.data) {
    other.size = 0;
    other.data = nullptr;
}

AllocBuffer& AllocBuffer::operator=(AllocBuffer&& other) {
    if(&other != this) {
        delete[] data;

        data = other.data;
        size = other.size;
        other.data = nullptr;
        other.size = 0;
    }
    return *this;
}

AllocBuffer AllocBuffer::copy(Buffer data) {
    AllocBuffer n {data.size};
    data.copy_to(n.as_buffer());
    return n;
}
