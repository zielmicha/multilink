#include "libreactor/reactor.h"
#include "libreactor/timer.h"

class ThrottledStream: public Stream {
    // Throttles only writes.
    Reactor& reactor;
    Timer timer;

    StreamPtr stream;
    double mbps;
    uint64_t next_transmission = 0;

    void write_ready();
    void read_ready();
    void error();
    bool can_transmit();

public:
    ThrottledStream(Reactor& reactor, StreamPtr stream, double mbps);
    ThrottledStream(const ThrottledStream& other) = delete;

    Buffer read(Buffer data);
    size_t write(const Buffer data);
    void close();

    STREAM_FIELDS
};

class DelayedStream: public Stream {
    // Delays writes.
    Reactor& reactor;
    Timer timer;
    StreamPtr stream;
    std::deque<AllocBuffer> buffers;
    int waiting = 0;
    int front_pointer = 0;
    uint64_t delay;
    uint64_t buffsize;

    void write_ready();
    void read_ready();
    void error();

    std::function<void()> after_delay_fn;

public:
    DelayedStream(Reactor& reactor, StreamPtr stream, uint64_t buffsize, uint64_t delay);
    DelayedStream(const DelayedStream& other) = delete;

    Buffer read(Buffer data);
    size_t write(const Buffer data);
    void close();

    STREAM_FIELDS
};
