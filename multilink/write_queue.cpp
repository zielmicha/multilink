#include "multilink/write_queue.h"
#define LOGGER_NAME "writequeue"
#include "libreactor/logging.h"

const int MTU = 4096; // TODO

WriteQueue::WriteQueue(Reactor& reactor, std::shared_ptr<PacketStream> output, size_t max_size) :
    output(output), queue(max_size, MTU) {
}

std::shared_ptr<WriteQueue> WriteQueue::create(Reactor& reactor, std::shared_ptr<PacketStream> output, size_t max_size) {
    auto ptr = std::shared_ptr<WriteQueue>(new WriteQueue(reactor, output, max_size));
    output->set_on_send_ready(std::bind(&WriteQueue::on_send_ready, ptr));
    return ptr;
}

bool WriteQueue::send(const Buffer data) {
    bool ok = queue.push_back(data);
    if(!ok) return false;
    on_send_ready();
    return true;
}

void WriteQueue::on_send_ready() {
    while(!queue.empty() && output->is_send_ready()) {
        output->send(queue.front());
        queue.pop_front();
    }
}

void write_null_packets(PacketStream* output, size_t packet_size) {
    output->set_on_send_ready([=]() {
        AllocBuffer buffer {packet_size};
        buffer.as_buffer().set_zero();

        while(output->is_send_ready()) output->send(buffer.as_buffer());
    });
}
