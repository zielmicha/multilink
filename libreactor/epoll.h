#ifndef EPOLL_H_
#define EPOLL_H_

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

#include <vector>

void setnonblocking(int fd);

struct EPollResult {
    int fd;
    bool read_ready;
    bool write_ready;
    bool error;
};

class EPoll;
class EPollIterable;

class EPollIterator {
private:
    friend class EPollIterable;

    const EPoll& epoll;
    int i;
    EPollIterator(const EPoll& epoll, int i);
public:
    bool operator!=(const EPollIterator& other) const;
    EPollResult operator*() const;
    EPollIterator& operator++();
};


class EPollIterable {
private:
    friend class EPoll;

    const EPoll& epoll;
    EPollIterable(const EPoll& epoll);
    EPollIterator at(int i) const;
public:
    EPollIterator begin() const;
    EPollIterator end() const;
};

class EPoll {
protected:
    friend class EPollIterator;
    friend class EPollIterable;

    int efd;
    std::vector<epoll_event> events;
    EPollIterable iterable;
    int event_count;
public:
    EPoll();
    EPoll(const EPoll& other) = delete;
    ~EPoll();
    bool add(int fd);
    const EPollIterable& wait();
};

#endif
