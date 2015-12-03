#ifndef PACKET_STREAM_UTIL_H_
#define PACKET_STREAM_UTIL_H_
#include "libreactor/packet_stream.h"
#include "libreactor/reactor.h"

class FreeWriterPacketStream: public AbstractPacketStream {
protected:
    Reactor& reactor;
    StreamPtr stream;

    AllocBuffer send_buffer;
    Buffer send_buffer_current;

    void write_ready();
    void error_ready();

    FreeWriterPacketStream(Reactor& reactor, StreamPtr stream);
public:
    FreeWriterPacketStream(const FreeWriterPacketStream&) = delete;

    void send_with_offset(const Buffer data);
    bool is_send_ready();
    size_t get_send_offset() { return 0; }

    void close() { stream->close(); stream = nullptr; }
};

class FreePacketStream: public FreeWriterPacketStream {
    AllocBuffer recv_buffer;

    void read_ready();

    FreePacketStream(Reactor& reactor, StreamPtr stream, size_t mtu);
public:
    FreePacketStream(const FreePacketStream&) = delete;

    static std::shared_ptr<FreePacketStream> create(Reactor& reactor, StreamPtr stream,
                                                    size_t mtu = 8192);

    optional<Buffer> recv();
};

class LengthPacketStream: public FreeWriterPacketStream {
    AllocBuffer recv_buffer;
    AllocBuffer recv_caller_buffer;
    int recv_buffer_pos;
    void read_ready();

    LengthPacketStream(Reactor& reactor, StreamPtr stream);
public:
    LengthPacketStream(const LengthPacketStream&) = delete;

    static std::shared_ptr<LengthPacketStream> create(Reactor& reactor, StreamPtr stream);

    void send(const Buffer data);
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

namespace packetutil {
    void pipe(Reactor& reactor,
              std::shared_ptr<PacketStream> in, std::shared_ptr<PacketStream> out,
              size_t mtu=8192);
    void pipe_both(Reactor& reactor,
              PacketStreamPtr a, PacketStreamPtr b,
              size_t mtu=8192);
    void pipe_both(Reactor& reactor,
                   StreamPtr a, StreamPtr b, size_t mtu=8192);
}

using packetutil::pipe;
using packetutil::pipe_both;

#endif
