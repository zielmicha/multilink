#ifndef PACKET_STREAM_H_
#define PACKET_STREAM_H_
#include "buffer.h"
#include <functional>
#include <boost/optional/optional.hpp>

template <class T>
using optional = boost::optional<T>;

class PacketStream {
public:
    virtual optional<Buffer> recv() = 0;
    // TODO: send should return void and abort if can't send
    virtual bool send(const Buffer data) = 0;
    virtual bool send_with_offset(Buffer data) = 0;
    virtual size_t get_send_offset() = 0;
    virtual bool is_send_ready() = 0;

    virtual void close() = 0;

    virtual void set_on_recv_ready(std::function<void()> f) = 0;
    virtual void set_on_send_ready(std::function<void()> f) = 0;
    virtual void set_on_error(std::function<void()> f) = 0;
};

class AbstractPacketStream: public PacketStream {
public:
    std::function<void()> on_recv_ready;
    std::function<void()> on_send_ready;
    std::function<void()> on_error;

    bool send(const Buffer data) {
        assert(get_send_offset() == 0);
        return send_with_offset(data);
    }

    void set_on_recv_ready(std::function<void()> f) {
        on_recv_ready = f;
    }
    void set_on_send_ready(std::function<void()> f) {
        on_send_ready = f;
    }
    void set_on_error(std::function<void()> f) {
        on_error = f;
    }
};

#endif
