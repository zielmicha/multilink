#ifndef BYTESTRING_H_
#include <memory>
#include "buffer.h"

class ByteString {
    std::shared_ptr<AllocBuffer> data;
public:
    ByteString();
    ByteString(int size);
    ByteString(const ByteString& other);

    ByteString copy() const;

    operator Buffer() const {
        return data->as_buffer();
    }

    operator std::string() const {
        return data->as_buffer();
    }

    Buffer as_buffer() const {
        return *this;
    }

    int size() const {
        return data->as_buffer().size;
    }
};

#endif
