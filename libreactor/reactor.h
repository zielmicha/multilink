#ifndef REACTOR_H_
#define REACTOR_H_
#include <memory>
#include <functional>
#include <unordered_map>
#include "epoll.h"
#include "buffer.h"

class Reactor;

void setnonblocking(int fd);

class Reactor;

class Stream {
public:
    virtual Buffer read(Buffer data) = 0;
    virtual size_t write(const Buffer data) = 0;
    virtual void close() = 0;

    virtual void set_on_read_ready(std::function<void()> f) = 0;
    virtual void set_on_write_ready(std::function<void()> f) = 0;
    virtual void set_on_error(std::function<void()> f) = 0;
};

class FD: public Stream {
private:
    friend class Reactor;
    Reactor& reactor;

    int fd;
    FD(Reactor& r, int fd);
public:
    FD(const FD& r) = delete;
    FD(FD&& r) = default;

    Buffer read(Buffer data);
    size_t write(const Buffer data);
    void close();

    std::function<void()> on_read_ready;
    std::function<void()> on_write_ready;
    std::function<void()> on_error;

    void set_on_read_ready(std::function<void()> f) {
        on_read_ready = f;
    }
    void set_on_write_ready(std::function<void()> f) {
        on_write_ready = f;
    }
    void set_on_error(std::function<void()> f) {
        on_error = f;
    }
};

class Reactor {
private:
    friend class FD;

    EPoll epoll;
    std::unordered_map<int, FD> fds;
    bool want_exit = false;
public:
    Reactor(const Reactor& r) = delete;
    Reactor();

    FD& take_fd(int fd);

    void step();
    void exit();
    void run();
};

#endif
