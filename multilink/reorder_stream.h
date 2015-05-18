#ifndef REORDER_STREAM_H_
#define REORDER_STREAM_H_
#include "packet_stream.h"
#include <unordered_map>

class ReorderStream: public AbstractPacketStream {
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
    bool send_with_offset(Buffer data);
    size_t get_send_offset() { return 8; }

    void close() { stream->close(); }
};

#endif
