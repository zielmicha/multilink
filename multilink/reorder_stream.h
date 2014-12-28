#ifndef REORDER_STREAM_H_
#define REORDER_STREAM_H_
#include "packet_stream.h"
#include <unordered_map>

class ReorderStream: public PacketStream {
    PacketStream* stream;

    void transport_recv_ready();
    void transport_send_ready();

    // TODO: remove allocations?
    // FIXME: packets may grow unbounded
    std::unordered_map<uint64_t, AllocBuffer> packets;
    uint64_t next_recv_seq = 1;
    uint64_t current_recv_seq = 0;
    uint64_t next_send_seq = 1;

public:
    ReorderStream(PacketStream* stream);

    optional<Buffer> recv();
    bool send(Buffer data);
    bool send_offset_8(Buffer data);

    void close() { stream->close(); }

    PACKET_STREAM_FIELDS
};

#endif
