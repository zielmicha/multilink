#include "multilink.h"
#include "misc.h"
#include <sys/types.h>
#include <sys/socket.h>

int main() {
    Reactor reactor;
    Multilink::Multilink left(reactor);
    Multilink::Multilink right(reactor);

    std::vector<FD*> fds = fd_pair(reactor);

    left.add_link(fds[0], "left");
    right.add_link(fds[1], "right");

    left.send(Buffer::from_cstr("foobar"));

    reactor.run();
}
