#include "misc.h"
#include "common.h"
#include <random>

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

const char* hex_digits = "0123456789abcdef";

std::string random_hex_string(int length) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 16);
    std::string out;
    out.resize(length);

    for (int i=0; i < length; i ++) {
        out[i] = hex_digits[dist(rd)];
    }
    return out;
}
