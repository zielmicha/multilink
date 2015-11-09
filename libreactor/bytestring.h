#ifndef BYTESTRING_H_
#define BYTESTRING_H_
#include <memory>
#include "buffer.h"

class ByteString {
    std::shared_ptr<AllocBuffer> data;
public:
    ByteString();
    ByteString(size_t size);
    ByteString(const ByteString& other);

    ByteString copy() const;

    static ByteString copy_from(Buffer buff);
    static ByteString copy_from(const char* cstring);

    operator Buffer() const {
        return data->as_buffer();
    }

    operator std::string() const {
        return data->as_buffer();
    }

    Buffer as_buffer() const {
        return *this;
    }

    size_t size() const {
        return data->as_buffer().size;
    }


    friend std::ostream& operator<<(std::ostream& stream, const ByteString& bs) {
        return stream << "ByteString" << bs.as_buffer();
    }
};

#endif
