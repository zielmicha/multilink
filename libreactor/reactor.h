#ifndef REACTOR_H_
#define REACTOR_H_
#include <memory>
#include <functional>
#include <unordered_map>
#include "libreactor/epoll.h"
#include "libreactor/buffer.h"

class Reactor;

void setnonblocking(int fd);
inline void nothing() {};

class Reactor;

class Stream {
public:
    virtual Buffer read(Buffer data) = 0;
    virtual size_t write(const Buffer data) = 0;
    virtual void close() = 0;

    virtual void set_on_read_ready(std::function<void()> f) = 0;
    virtual void set_on_write_ready(std::function<void()> f) = 0;
    virtual void set_on_error(std::function<void()> f) = 0;

    void set_on_read_ready_and_schedule(Reactor& r, std::function<void()> f);
    void set_on_write_ready_and_schedule(Reactor& r, std::function<void()> f);
};

#define STREAM_FIELDS                                   \
    std::function<void()> on_read_ready;                \
    std::function<void()> on_write_ready;               \
    std::function<void()> on_error;                     \
                                                        \
    void set_on_read_ready(std::function<void()> f) {   \
        on_read_ready = f;                              \
    }                                                   \
    void set_on_write_ready(std::function<void()> f) {  \
        on_write_ready = f;                             \
    }                                                   \
    void set_on_error(std::function<void()> f) {        \
        on_error = f;                                   \
    }

class AbstractStream: public Stream {
public:
    AbstractStream():
        on_read_ready(nothing),
        on_write_ready(nothing),
        on_error(nothing) {}

    STREAM_FIELDS
};

class FD: public Stream {
private:
    friend class Reactor;
    Reactor& reactor;

    int fd;
    FD(Reactor& r, int fd);

    bool error_reported = false;
    void handle_error(bool use_errno);

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

    void set_close_on_exec(bool flag);

    STREAM_FIELDS
};

using FDPtr = std::shared_ptr<FD>;
using StreamPtr = std::shared_ptr<Stream>;

class Reactor {
private:
    friend class FD;

    EPoll epoll;
    std::unordered_map<int, FDPtr> fds;
    std::vector<std::function<void()> > scheduled_functions;
    bool want_exit = false;
    uint64_t epoch = 0;
public:
    Reactor(const Reactor& r) = delete;
    Reactor();

    FDPtr take_fd(int fd);

    void schedule(std::function<void()> func);

    void step();
    void exit();
    void run();
};

#endif
