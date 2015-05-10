#ifndef REACTOR_H_
#define REACTOR_H_
#include <memory>
#include <functional>
#include <unordered_map>
#include "epoll.h"
#include "buffer.h"
#include "function.h"

class Reactor;

void setnonblocking(int fd);

class Reactor;

class Stream {
public:
    virtual Buffer read(Buffer data) = 0;
    virtual size_t write(const Buffer data) = 0;
    virtual void close() = 0;

    virtual void set_on_read_ready(function<void()> f) = 0;
    virtual void set_on_write_ready(function<void()> f) = 0;
    virtual void set_on_error(function<void()> f) = 0;
};

// There must be a better way...
#define STREAM_FIELDS                                   \
    function<void()> on_read_ready;                \
    function<void()> on_write_ready;               \
    function<void()> on_error;                     \
                                                        \
    void set_on_read_ready(function<void()> f) {   \
        on_read_ready = f;                              \
    }                                                   \
    void set_on_write_ready(function<void()> f) {  \
        on_write_ready = f;                             \
    }                                                   \
    void set_on_error(function<void()> f) {        \
        on_error = f;                                   \
    }

class FD: public Stream {
private:
    friend class Reactor;
    Reactor& reactor;

    int fd;
    FD(Reactor& r, int fd);

#ifdef ENABLE_EPOCH_LIMIT
    uint64_t last_epoch = 0;
    uint64_t epoch_read_bytes = 0;
#endif
public:
    FD(const FD& r) = delete;
    FD(FD&& r) = default;

    Buffer read(Buffer data);
    size_t write(const Buffer data);
    void close();
    int fileno();

    STREAM_FIELDS
};

class Reactor {
private:
    friend class FD;

    EPoll epoll;
    std::unordered_map<int, FD> fds;
    std::vector<function<void()> > scheduled_functions;
    bool want_exit = false;
    uint64_t epoch = 0;
public:
    Reactor(const Reactor& r) = delete;
    Reactor();

    FD& take_fd(int fd);

    void schedule(function<void()> func);

    void step();
    void exit();
    void run();
};

#endif
