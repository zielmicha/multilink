#include "write_queue.h"
#include "logging.h"

WriteQueue::WriteQueue(Reactor& reactor, PacketStream* output, size_t max_size) :
    output(output), queue(max_size) {
    output->set_on_send_ready(std::bind(&WriteQueue::on_send_ready, this));
}

bool WriteQueue::send(const Buffer data) {
    bool ok = queue.push_back(data);
    if(!ok) return false;
    on_send_ready();
    return true;
}

void WriteQueue::on_send_ready() {
    while(!queue.empty() && output->send(queue.front()))
        queue.pop_front();
}

void write_null_packets(PacketStream* output, size_t packet_size) {
    output->set_on_send_ready([=]() {
        AllocBuffer buffer {packet_size};
        buffer.as_buffer().set_zero();

        while(output->send(buffer.as_buffer())) ;
    });
}
