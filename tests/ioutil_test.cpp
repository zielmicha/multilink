#include "libreactor/ioutil.h"
#include "libreactor/misc.h"
#include <iostream>

int main() {
    Reactor reactor;

    std::vector<FDPtr> fds = fd_pair(reactor);

    ioutil::write(fds[0], ByteString::copy_from("hello")).wait(reactor);
    ioutil::read(fds[1], 4).then<ByteString>([&](ByteString ret) {
        std::cerr << ret << std::endl;
        return ioutil::read(fds[1], 1);
    }).then<unit>([](ByteString ret) {
        std::cerr << ret << std::endl;
        return unit();
    }).wait(reactor);

    exit(0);
}
