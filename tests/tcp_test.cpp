#include "libreactor/logging.h"
#include "libreactor/tcp.h"

void test_read(FD* fd) {
    while(true) {
        StackBuffer<10> buff;
        Buffer data = fd->read(buff);
        std::cout << url_encode(data) << std::endl;
        std::cout << "read bytes=" << data.size << std::endl;
        if(data.size == 0) break;
    }
}

int main() {
    Reactor reactor;
    TCP::connect(reactor, "127.0.0.1", 8000).on_success_or_failure(
        [](FD* fd) {
            LOG("connected");
            fd->on_read_ready = [fd]() {
                test_read(fd);
            };
        },
        [](std::unique_ptr<std::exception> ex) {
            LOG("connection failed: " << ex->what());
        });

    TCP::listen(reactor, "127.0.0.1", 8002, [](FD* fd) {
            LOG("client connected");
            fd->on_read_ready = [fd]() {
                test_read(fd);
            };
        }).ignore();

    reactor.run();
}
