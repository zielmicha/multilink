#include "reactor.h"
#include "epoll.h"
#include "signals.h"
#include "common.h"
#include <unistd.h>
#include <assert.h>
#include <algorithm>
#define LOGGER_NAME "reactor"
#include "logging.h"

size_t FD::write(const Buffer data) {
    int ret = ::write(fd, data.data, data.size);
    if(ret < 0) {
        if(errno == EAGAIN)
            return 0;
        else
            errno_to_exception();
    } else {
        return (size_t)ret;
    }
}

size_t FD::read(Buffer data) {
    int ret = ::read(fd, data.data, data.size);
    if(ret < 0) {
        if(errno == EAGAIN)
            return 0;
        else
            errno_to_exception();
    } else {
        return (size_t)ret;
    }
}

void FD::close() {
    ::close(fd);
    // commit suicide
    reactor.fds.erase(fd);
}

void nop() {}

FD::FD(Reactor& r, int fd): reactor(r), fd(fd), on_read_ready(nop), on_write_ready(nop), on_error(nop) {}

FD& Reactor::take_fd(int fd) {
    setnonblocking(fd);
    fds.insert(decltype(fds)::value_type(fd, FD(*this, fd)));
    // FIXME: don't throw exception (return invalid fd or error monad?)
    if(!epoll.add(fd)) errno_to_exception();
    return fds.find(fd)->second;
}

Reactor::Reactor() {}

void Reactor::step() {
    auto& result = epoll.wait();
    Signals::call_handlers();

    for(EPollResult r: result) {
        auto iter = fds.find(r.fd);
        if(iter == fds.end()) {
            // TODO: may epoll return event for already closed FD?
            LOG("epoll returned unknown fd: " << r.fd);
        }
        FD& fd = iter->second;
        if(r.read_ready)
            fd.on_read_ready();
        if(r.write_ready)
            fd.on_write_ready();
        if(r.error)
            fd.on_error();
    }
}

void Reactor::run() {
    while(!want_exit) step();
}

void Reactor::exit() {
    want_exit = true;
}
