#include "packet_queue.h"
#define LOGGER_NAME "packetqueue"
#include "logging.h"

PacketQueue::PacketQueue(size_t max_size, size_t mtu): mtu(mtu), max_count(max_size / mtu) {
}

void PacketQueue::pop_front() {
    queue.pop_front();
}

Buffer PacketQueue::front() {
    return queue.front().as_buffer();
}

bool PacketQueue::is_full() {
    return queue.size() >= max_count;
}

bool PacketQueue::push_back(const Buffer data) {
    if(is_full()) return false;
    queue.push_back(AllocBuffer::copy(data));
    return true;
}
