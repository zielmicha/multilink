#include "buffer.h"
#include <cassert>
#include <sstream>
#include <iomanip>

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
