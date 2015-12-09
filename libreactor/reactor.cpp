#include "libreactor/reactor.h"
#include "libreactor/epoll.h"
#include "libreactor/signals.h"
#include "libreactor/common.h"
#include <unistd.h>
#include <assert.h>
#include <algorithm>
#define LOGGER_NAME "reactor"
#include "libreactor/logging.h"

#ifdef ENABLE_EPOCH_LIMIT
const uint64_t MAX_EPOCH_READ = 40960;
const uint64_t MAX_EPOCH_WRITE = 40960;
const uint64_t EPOCH_INF = 1ull << 63;
#endif

void Stream::set_on_read_ready_and_schedule(Reactor& r, std::function<void()> f) {
    set_on_read_ready(f);
    r.schedule(f);
}

void Stream::set_on_write_ready_and_schedule(Reactor& r, std::function<void()> f) {
    set_on_write_ready(f);
    r.schedule(f);
}

size_t FD::write(const Buffer data) {
    assert(data.size != 0);
    int ret = ::write(fd, data.data, data.size);
    if(ret < 0) {
        if(errno != EAGAIN)
            handle_error(true);

        return 0;
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
    if(ret <= 0) { // zero byte read indicates shutdown
        if(errno != EAGAIN)
            handle_error(true);

        return data.slice(0, 0);
    } else {
        return data.slice(0, (size_t)ret);
    }
}

void FD::handle_error(bool use_errno) {
    if(error_reported) return;
    error_reported = true;

    reactor.schedule(on_error);
}

int FD::fileno() {
    return fd;
}

void FD::close() {
    ::close(fd);
    fd = -1;
    // commit suicide
    reactor.fds.erase(fd);
}

void FD::set_close_on_exec(bool flag) {
    int flags = fcntl(fd, F_GETFD);

    if (flag)
        flags |= FD_CLOEXEC;
    else
        flags &= ~FD_CLOEXEC;

    fcntl(fd, F_SETFD, flags);
}

void nop() {}

FD::FD(Reactor& r, int fd): reactor(r), fd(fd), on_read_ready(nop), on_write_ready(nop), on_error(nop) {}

FDPtr Reactor::take_fd(int fd) {
    setnonblocking(fd);

    FDPtr fd_obj = FDPtr(new FD(*this, fd));
    fds[fd] = fd_obj;

    if(!epoll.add(fd)) errno_to_exception();
    fd_obj->set_close_on_exec(true);
    return fd_obj;
}

Reactor::Reactor() {
    Signals::ignore_signal(SIGPIPE);
}

void Reactor::schedule(std::function<void()> func) {
    assert(scheduled_functions.size() < 1000);
    scheduled_functions.push_back(func);
}

void Reactor::step() {
    epoch ++;

    decltype(scheduled_functions) scheduled_functions_copy;
    std::swap(scheduled_functions, scheduled_functions_copy);
    for(auto& func: scheduled_functions_copy)
        func();

    int timeout;

    if(scheduled_functions.empty())
        timeout = -1; // block indefinitely
    else
        timeout = 0; // don't block

    auto& result = epoll.wait(timeout);
    Signals::call_handlers();

    for(EPollResult r: result) {
        auto iter = fds.find(r.fd);
        if(iter == fds.end()) {
            // TODO: may epoll return event for already closed FD?
            LOG("epoll returned unknown fd: " << r.fd);
            continue;
        }
        FDPtr& fd = iter->second;
        if(r.read_ready)
            fd->on_read_ready();
        if(r.write_ready)
            fd->on_write_ready();
        if(r.error)
            fd->handle_error(false);
    }

    DEBUG("step finished");
}

void Reactor::run() {
    while(!want_exit) step();
}

void Reactor::exit() {
    want_exit = true;
}
