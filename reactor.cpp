#include "reactor.h"
#include "epoll.h"
#include "common.h"
#include <unistd.h>
#include <algorithm>

size_t FD::write(const char* data, size_t length) {
    int ret = ::write(fd, data, length);
    if(ret < 0) {
        if(errno == EAGAIN)
            return 0;
        else
            errno_to_exception();
    } else {
        return (size_t)ret;
    }
}

size_t FD::read(char* target, size_t length) {
    int ret = ::read(fd, target, length);
    if(ret < 0) {
        if(errno == EAGAIN)
            return 0;
        else
            errno_to_exception();
    } else {
        return (size_t)ret;
    }
}

void nop() {}

FD::FD(int fd): fd(fd), on_read_ready(nop), on_write_ready(nop), on_error(nop) {}

FD& Reactor::take_fd(int fd) {
    setnonblocking(fd);
    fds.insert(decltype(fds)::value_type(fd, FD(fd)));
    epoll.add(fd);
    return fds.find(fd)->second;
}

Reactor::Reactor() {}

void Reactor::step() {
    auto& result = epoll.wait();
    for(EPollResult r: result) {
        FD& fd = fds.find(r.fd)->second;
        if(r.read_ready)
            fd.on_read_ready();
        if(r.write_ready)
            fd.on_write_ready();
        if(r.error)
            fd.on_error();
    }
}

void Reactor::run() {
    while(true) step();
}
