#include "misc.h"

std::vector<FD*> fd_pair(Reactor& reactor) {
    int rawfds[2];
    socketpair(PF_UNIX, SOCK_STREAM, 0, rawfds);

    return {
        &reactor.take_fd(rawfds[0]),
        &reactor.take_fd(rawfds[1])
    };
}
