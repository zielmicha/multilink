#ifndef WRITE_QUEUE_H_
#define WRITE_QUEUE_H_
#include "libreactor/reactor.h"
#include "libreactor/packet_stream.h"
#include "multilink/packet_queue.h"

class WriteQueue {
    std::shared_ptr<PacketStream> output;
    PacketQueue queue;

    void on_send_ready();

    WriteQueue(Reactor& reactor, std::shared_ptr<PacketStream> output, size_t max_size);
public:

    bool send(const Buffer data);

    static std::shared_ptr<WriteQueue> create(
        Reactor& reactor,
        std::shared_ptr<PacketStream> output,
        size_t max_size);
};

void write_null_packets(PacketStream* output, size_t packet_size);

#endif
