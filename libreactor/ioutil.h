#ifndef IOUTIL_H_
#define IOUTIL_H_
#include "future.h"
#include "reactor.h"

namespace ioutil {
    Future<Buffer> read(FD* fd, int size);
    Future<unit> write(FD* fd, Buffer data);
}

#endif
