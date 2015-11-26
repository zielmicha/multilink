#ifndef PACKET_QUEUE_H_
#define PACKET_QUEUE_H_
#include <deque>
#include "libreactor/buffer.h"

class PacketQueue {
    std::deque<AllocBuffer> queue;
    const size_t mtu;

public:
    const size_t max_count;

    PacketQueue(size_t max_size, size_t mtu);
    Buffer front();
    void pop_front();
    int packet_count() { return queue.size(); }
    size_t size_of(int i) { return queue[i].as_buffer().size; }

    bool push_back(const Buffer data);
    bool empty() { return queue.empty();  }
    bool is_full();
};

#endif
