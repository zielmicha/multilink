#include "reactor.h"
#include "epoll.h"
#include "signals.h"
#include "common.h"
#include <unistd.h>
#include <assert.h>
#include <algorithm>
#define LOGGER_NAME "reactor"
#include "logging.h"

#ifdef ENABLE_EPOCH_LIMIT
const uint64_t MAX_EPOCH_READ = 40960;
const uint64_t EPOCH_INF = 1ull << 63;
#endif

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

Buffer FD::read(Buffer data) {
    #ifdef ENABLE_EPOCH_LIMIT
    if(reactor.epoch == last_epoch) {
        last_epoch = reactor.epoch;
        epoch_read_bytes = 0;
    }

    if(epoch_read_bytes > MAX_EPOCH_READ) {
        if(epoch_read_bytes != EPOCH_INF) {
            reactor.schedule(on_read_ready);
            epoch_read_bytes = EPOCH_INF;
        }
        return data.slice(0, 0);
    }
    #endif

    int ret = ::read(fd, data.data, data.size);
    #ifdef ENABLE_EPOCH_LIMIT
    epoch_read_bytes += ret;
    #endif
    if(ret < 0) {
        if(errno == EAGAIN)
            return data.slice(0, 0);
        else
            errno_to_exception();
    } else {
        return data.slice(0, (size_t)ret);
    }
}

int FD::fileno() {
    return fd;
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

void Reactor::schedule(std::function<void()> func) {
    assert(scheduled_functions.size() < 1000);
    scheduled_functions.push_back(func);
}

void Reactor::step() {
    epoch ++;

    while(!scheduled_functions.empty()) {
        decltype(scheduled_functions) scheduled_functions_copy;
        std::swap(scheduled_functions, scheduled_functions_copy);
        for(auto& func: scheduled_functions_copy)
            func();
    }

    auto& result = epoll.wait();
    Signals::call_handlers();

    for(EPollResult r: result) {
        auto iter = fds.find(r.fd);
        if(iter == fds.end()) {
            // TODO: may epoll return event for already closed FD?
            LOG("epoll returned unknown fd: " << r.fd);
            continue;
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
