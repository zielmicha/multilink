#ifndef IOUTIL_H_
#define IOUTIL_H_
#include "future.h"
#include "reactor.h"
#include "bytestring.h"

namespace ioutil {
    Future<ByteString> read(Stream* fd, int size);
    Future<unit> write(Stream* fd, ByteString data);
}

#endif
