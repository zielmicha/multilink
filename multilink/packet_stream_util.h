#ifndef PACKET_STREAM_UTIL_H_
#define PACKET_STREAM_UTIL_H_
#include "packet_stream.h"
#include "reactor.h"

class FreePacketStream: public PacketStream {
    Reactor& reactor;
    Stream* stream;

    AllocBuffer send_buffer;
    Buffer send_buffer_current;
    AllocBuffer recv_buffer;

    void write_ready();
    void read_ready();

    FreePacketStream(Reactor& reactor, Stream* stream);
public:
    FreePacketStream(const FreePacketStream&) = delete;

    static std::shared_ptr<FreePacketStream> create(Reactor& reactor, Stream* stream);

    optional<Buffer> recv();
    bool send(const Buffer data);
    void close() { stream->close(); }

    PACKET_STREAM_FIELDS
};


class Piper {
    Reactor& reactor;
    PacketStream* in;
    PacketStream* out;

    void recv_ready();
    void send_ready();

    AllocBuffer buffer;
    optional<Buffer> current;
public:
    Piper(Reactor& reactor,
          PacketStream* in,
          PacketStream* out);
};

void pipe(Reactor& reactor,
          PacketStream* in,
          PacketStream* out);

#endif
