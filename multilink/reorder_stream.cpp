#include "reorder_stream.h"
#define LOGGER_NAME "reorderstream"
#include "logging.h"

ReorderStream::ReorderStream(PacketStream* stream): stream(stream) {
    stream->set_on_recv_ready(std::bind(&ReorderStream::transport_recv_ready, this));
    stream->set_on_send_ready(std::bind(&ReorderStream::transport_send_ready, this));
}

bool ReorderStream::send_with_offset(Buffer data) {
    data.convert<uint64_t>(0) = (next_send_seq);
    if(stream->send(data)) {
        next_send_seq ++;
        return true;
    } else {
        return false;
    }
}

void ReorderStream::transport_send_ready() {
    on_send_ready();
}

void ReorderStream::transport_recv_ready() {
    bool front_received = false;
    while(true) {
        optional<Buffer> data = stream->recv();
        if(!data) break;
        Buffer buff = *data;
        assert(buff.size >= 8);
        uint64_t seq = buff.convert<uint64_t>(0);
        Buffer user_data = buff.slice(8);

        if(seq < next_recv_seq) continue;

        AllocBuffer final_buff(user_data.size);
        user_data.copy_to(final_buff.as_buffer());
        packets.insert(std::make_pair(seq, std::move(final_buff)));
        if(seq == next_recv_seq) front_received = true;
    }
    if(front_received)
        on_recv_ready();
}

optional<Buffer> ReorderStream::recv() {
    auto it = packets.find(next_recv_seq);
    if(it != packets.end()) {
        Buffer retvalue = it->second.as_buffer();

        if(current_recv_seq != 0)
            packets.erase(current_recv_seq);

        next_recv_seq ++;
        current_recv_seq ++;
        return retvalue;
    }
    return optional<Buffer>();
}
