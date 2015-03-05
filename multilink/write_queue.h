#ifndef WRITE_QUEUE_H_
#define WRITE_QUEUE_H_
#include "reactor.h"
#include "packet_stream.h"
#include "packet_queue.h"

class WriteQueue {
    PacketStream* output;
    PacketQueue queue;

    void on_send_ready();

public:
    WriteQueue(Reactor& reactor, PacketStream* output, size_t max_size);

    bool send(const Buffer data);
};

#endif
