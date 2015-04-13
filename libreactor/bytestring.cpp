#include "bytestring.h"
ByteString::ByteString(): data(nullptr) {}

ByteString::ByteString(int size) {
    data = std::make_shared<AllocBuffer>(size);
}

ByteString::ByteString(const ByteString& other) {
    data = other.data;
}

ByteString ByteString::copy() const {
    if(data) {
        ByteString n {size()};
        data->as_buffer().copy_to(n.data->as_buffer());
        return n;
    } else {
        return ByteString();
    }
}
