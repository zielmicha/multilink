#include "multilink.h"
#include <sys/types.h>
#include <sys/socket.h>

int main() {
    Reactor reactor;
    Multilink::Multilink left;
    Multilink::Multilink right;

    int rawfds[2];
    socketpair(PF_UNIX, SOCK_STREAM, 0, rawfds);

    std::vector<FD*> fds = {
        &reactor.take_fd(rawfds[0]),
        &reactor.take_fd(rawfds[1])
    };

    left.add_link(fds[0], "left");
    right.add_link(fds[1], "right");

    Multilink::ChannelInfo info;
    info.type = Multilink::ChannelInfo::TYPE_COMMON;
    left.send(info, "foobar", 7);
}
