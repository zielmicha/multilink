#ifndef IOUTIL_H_
#define IOUTIL_H_
#include "future.h"
#include "reactor.h"

namespace ioutil {
    Future<Buffer> read(Stream* fd, int size);
    Future<unit> write(Stream* fd, Buffer data);
}

#endif
