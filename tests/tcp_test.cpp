#include "logging.h"
#include "tcp.h"

void test_read(FD* fd) {
    while(true) {
        char buff[10];
        auto size = fd->read(buff, sizeof(buff));
        std::cout << url_encode(buff) << std::endl;
        std::cout << "read bytes=" << size << std::endl;
        if(size == 0) break;
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
