#include "buffer.h"
#include <cassert>
#include <memory.h>

Buffer Buffer::slice(size_t start, size_t size) {
    assert(size >= 0 && start >= 0);
    assert(start + size < this->size);

    return Buffer(data + start, size);
}

void Buffer::copy_to(Buffer target) const {
    assert(size <= target.size);
    memcpy(target.data, data, size);
}

AllocBuffer::AllocBuffer(size_t size) {
    data = new char[size];
    size = size;
}

AllocBuffer::~AllocBuffer() {
    delete[] data;
}

Buffer AllocBuffer::as_buffer() {
    return Buffer(data, size);
}
