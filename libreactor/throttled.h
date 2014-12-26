#include "reactor.h"
#include "timer.h"

class ThrottledStream: public Stream {
    // Throttles only writes.
    Reactor& reactor;
    Timer timer;

    Stream* stream;
    double mbps;
    uint64_t next_transmission = 0;

    void write_ready();
    void read_ready();
    void error();

public:
    ThrottledStream(Reactor& reactor, Stream* stream, double mbps);
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
    Stream* stream;
    std::deque<AllocBuffer> buffers;
    int waiting = 0;
    int front_pointer = 0;
    uint64_t delay;
    uint64_t buffsize;

    void write_ready();
    void read_ready();
    void error();

public:
    DelayedStream(Reactor& reactor, Stream* stream, uint64_t buffsize, uint64_t delay);
    DelayedStream(const DelayedStream& other) = delete;

    Buffer read(Buffer data);
    size_t write(const Buffer data);
    void close();

    STREAM_FIELDS
};
