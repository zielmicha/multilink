#include "multilink.h"
#include "misc.h"
#include <sys/types.h>
#include <sys/socket.h>

int main() {
    Reactor reactor;
    Multilink::Multilink left;
    Multilink::Multilink right;

    std::vector<FD*> fds = fd_pair(reactor);

    left.add_link(fds[0], "left");
    right.add_link(fds[1], "right");

    Multilink::ChannelInfo info;
    info.type = Multilink::ChannelInfo::TYPE_COMMON;
    left.send(info, "foobar", 7);
}
