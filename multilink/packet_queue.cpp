#include "packet_queue.h"
#define LOGGER_NAME "packetqueue"
#include "logging.h"

PacketQueue::PacketQueue(size_t max_size): max_size(max_size),
                                           buffer(max_size)
{

}

void PacketQueue::pop_front() {
    queue.pop_front();
    if(queue.empty()) {
        end = start = 0;
    } else {
        start = queue.front().first;
    }
}

Buffer PacketQueue::front() {
    auto b = queue.back();
    return buffer.as_buffer().slice(b.first, b.second - b.first);
}

bool PacketQueue::push_back(const Buffer data) {
    size_t new_end = end + data.size;
    if(new_end > max_size) {
        new_end = data.size;
        if(new_end > start)
            return false;
    }
    bool iswrapped = end < start;
    if(end == start && !queue.empty()) iswrapped = true;
    if(iswrapped && new_end > start)
        return false;

    data.copy_to(buffer.as_buffer().slice(new_end - data.size, data.size));
    end = new_end;
    queue.push_back(std::make_pair(end - data.size, end));

    return true;
}
