#include "buffer.h"
#include <cassert>

Buffer Buffer::slice(size_t start, size_t size) {
    assert(size >= 0 && start >= 0);
    assert(start + size < this->size);

    return Buffer(data + start, size);
}
