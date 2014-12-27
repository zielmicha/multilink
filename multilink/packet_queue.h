#ifndef PACKET_QUEUE_H_
#define PACKET_QUEUE_H_
#include <deque>
#include "buffer.h"

class PacketQueue {
    uint64_t max_size;

    AllocBuffer buffer;
    size_t start = 0;
    size_t end = 0;

    std::deque<std::pair<size_t, size_t> > queue;

public:
    PacketQueue(size_t max_size);
    Buffer front();
    void pop_front();

    bool push_back(const Buffer data);
};

#endif
