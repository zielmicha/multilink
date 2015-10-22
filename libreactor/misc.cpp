#include "misc.h"
#include "common.h"

std::vector<FD*> fd_pair(Reactor& reactor) {
    int rawfds[2];
    socketpair(PF_UNIX, SOCK_STREAM, 0, rawfds);

    return {
        &reactor.take_fd(rawfds[0]),
        &reactor.take_fd(rawfds[1])
    };
}



void set_recv_buffer(FD* fd, int size) {
    int ret = setsockopt(fd->fileno(), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    if(ret < 0)
        errno_to_exception();
}
