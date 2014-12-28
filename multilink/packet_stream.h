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
    virtual bool send(const Buffer data) = 0;
    virtual void close() = 0;

    virtual void set_on_recv_ready(std::function<void()> f) = 0;
    virtual void set_on_send_ready(std::function<void()> f) = 0;
    virtual void set_on_error(std::function<void()> f) = 0;
};

// There must be a better way...
#define PACKET_STREAM_FIELDS                            \
    std::function<void()> on_recv_ready;                \
    std::function<void()> on_send_ready;                \
    std::function<void()> on_error;                     \
                                                        \
    void set_on_recv_ready(std::function<void()> f) {   \
        on_recv_ready = f;                              \
    }                                                   \
    void set_on_send_ready(std::function<void()> f) {   \
        on_send_ready = f;                              \
    }                                                   \
    void set_on_error(std::function<void()> f) {        \
        on_error = f;                                   \
    }                                                   \


#endif
