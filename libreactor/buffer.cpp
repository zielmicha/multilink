#include "buffer.h"
#include <cassert>
#include <sstream>
#include <iomanip>
#include <memory.h>

Buffer Buffer::slice(size_t start, size_t size) {
    assert(size >= 0 && start >= 0);
    assert(start + size <= this->size);

    return Buffer(data + start, size);
}

Buffer Buffer::slice(size_t start) {
    return slice(start, size - start);
}

void Buffer::copy_to(Buffer target) const {
    assert(size <= target.size);
    memcpy(target.data, data, size);
}

void Buffer::delete_start(size_t end) {
    assert(size >= end);
    memmove(data, data + end, size - end);
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

AllocBuffer::~AllocBuffer() {
    delete[] data;
}

Buffer AllocBuffer::as_buffer() {
    return Buffer(data, size);
}
