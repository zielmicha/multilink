#include "bytestring.h"
ByteString::ByteString(): data(nullptr) {}

ByteString::ByteString(size_t size) {
    data = std::make_shared<AllocBuffer>(size);
}

ByteString ByteString::copy_from(const char* c_string) {
    return copy_from(Buffer::from_cstr(c_string));
}

ByteString ByteString::copy_from(const Buffer buffer) {
    ByteString s {buffer.size};
    buffer.copy_to(s);
    return s;
}

ByteString ByteString::copy_from(const std::string& s) {
    return copy_from(Buffer(const_cast<char*>(s.data()), s.size()));
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
