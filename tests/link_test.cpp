#include "multilink.h"
#include "misc.h"
using namespace Multilink;

int main() {
    Reactor reactor;

    std::vector<FD*> fds = fd_pair(reactor);
    Link a {fds[0]};
    Link b {fds[1]};
}
