#ifndef IOUTIL_H_
#define IOUTIL_H_
#include "future.h"
#include "reactor.h"
#include "bytestring.h"
#include "packet_stream.h"

namespace ioutil {
    Future<ByteString> read(StreamPtr fd, int size);
    Future<unit> write(StreamPtr fd, ByteString data);
    Future<unit> send(PacketStreamPtr stream, ByteString data);
}

#endif
