#include "epoll.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

#define LOGGER_NAME "epoll"
#include "logging.h"

const int MAX_EVENTS = 64;

void setnonblocking(int sfd) {
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1) errno_to_exception();

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1) errno_to_exception();
}

EPoll::EPoll(): iterable(*this) {
    efd = epoll_create1(0);
    events.resize(MAX_EVENTS);
}

EPoll::~EPoll() {
    close(efd);
}

bool EPoll::add(int fd) {
    struct epoll_event event;
    event.data.u64 = 0; // supress valgrind error
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    int res = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    return res != -1;
}

const EPollIterable& EPoll::wait(int timeout) {
    sigset_t sigset;
    sigemptyset(&sigset);
    event_count = epoll_pwait(efd, &events[0], events.size(), timeout, &sigset);
    if(event_count < 0) {
        if(errno == EINTR)
            event_count = 0;
        else
            errno_to_exception();
    }
    return iterable;
}

EPollIterable::EPollIterable(const EPoll& epoll): epoll(epoll) {}

EPollIterator EPollIterable::begin() const {
    return at(0);
}

EPollIterator EPollIterable::end() const {
    return at(epoll.event_count);
}

EPollIterator EPollIterable::at(int i) const {
    return EPollIterator(epoll, i);
}

EPollIterator::EPollIterator(const EPoll& epoll, int i): epoll(epoll), i(i) {}

EPollIterator& EPollIterator::operator++() {
    i ++;
    return *this;
}

bool EPollIterator::operator!=(const EPollIterator& other) const {
    return other.i != i;
}

EPollResult EPollIterator::operator*() const {
    EPollResult result;
    int events = epoll.events[i].events;
    result.read_ready = events & EPOLLIN;
    result.write_ready = events & EPOLLOUT;
    result.error = events & (EPOLLOUT | EPOLLHUP);
    result.fd = epoll.events[i].data.fd;
    return result;
}
