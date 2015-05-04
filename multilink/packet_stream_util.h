#ifndef PACKET_STREAM_UTIL_H_
#define PACKET_STREAM_UTIL_H_
#include "packet_stream.h"
#include "reactor.h"

class FreeWriterPacketStream: public PacketStream {
protected:
    Reactor& reactor;
    Stream* stream;

    AllocBuffer send_buffer;
    Buffer send_buffer_current;

    void write_ready();

    FreeWriterPacketStream(Reactor& reactor, Stream* stream);
public:
    FreeWriterPacketStream(const FreeWriterPacketStream&) = delete;

    bool send(const Buffer data);

    PACKET_STREAM_FIELDS
};

class FreePacketStream: public FreeWriterPacketStream {
    AllocBuffer recv_buffer;

    void read_ready();

    FreePacketStream(Reactor& reactor, Stream* stream);
public:
    FreePacketStream(const FreePacketStream&) = delete;

    static std::shared_ptr<FreePacketStream> create(Reactor& reactor, Stream* stream);

    optional<Buffer> recv();
    void close() { stream->close(); }
};

class LengthPacketStream: public FreeWriterPacketStream {
    AllocBuffer recv_buffer;
    AllocBuffer recv_caller_buffer;
    int recv_buffer_pos;
    void read_ready();

    LengthPacketStream(Reactor& reactor, Stream* stream);
public:
    LengthPacketStream(const LengthPacketStream&) = delete;

    static std::shared_ptr<LengthPacketStream> create(Reactor& reactor, Stream* stream);

    bool send(const Buffer data);
    optional<Buffer> recv();
    void close() { stream->close(); }
};

class Piper {
    Reactor& reactor;
    std::shared_ptr<PacketStream> in;
    std::shared_ptr<PacketStream> out;

    void recv_ready();
    void send_ready();

    AllocBuffer buffer;
    optional<Buffer> current;

    Piper(Reactor& reactor,
          std::shared_ptr<PacketStream> in,
          std::shared_ptr<PacketStream> out,
          size_t mtu);
public:
    static std::shared_ptr<Piper> create(Reactor& reactor,
                                         std::shared_ptr<PacketStream> in,
                                         std::shared_ptr<PacketStream> out,
                                         size_t mtu);
};

void pipe(Reactor& reactor,
          std::shared_ptr<PacketStream> in, std::shared_ptr<PacketStream> out,
          size_t mtu=4096);
void pipe(Reactor& reactor,
          std::shared_ptr<Stream> in, std::shared_ptr<Stream> out);

#endif
