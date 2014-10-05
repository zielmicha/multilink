#ifndef REACTOR_H_
#define REACTOR_H_
#include <memory>
#include <functional>
#include <unordered_map>
#include "epoll.h"

class Reactor;

void setnonblocking(int fd);

class Reactor;

class FD {
private:
    friend class Reactor;

    int fd;
    FD(int fd);
public:
    FD(const FD& r) = delete;
    FD(FD&& r) = default;
    std::function<void()> on_read_ready;
    std::function<void()> on_write_ready;
    std::function<void()> on_error;

    size_t read(char* dest, size_t size);
    size_t write(const char* data, size_t size);
    void close();
};

class Reactor {
private:
    EPoll epoll;
    std::unordered_map<int, FD> fds;
public:
    Reactor(const Reactor& r) = delete;
    Reactor();

    FD& take_fd(int fd);

    void step();
    void run();
};

#endif
