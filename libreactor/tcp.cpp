#include "libreactor/tcp.h"
#include "libreactor/common.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define LOGGER_NAME "tcp"
#include "libreactor/logging.h"

namespace TCP {
    struct sockaddr_in make_addr(std::string addr, int port) {
        struct sockaddr_in servaddr = {0};
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(addr.c_str());
        servaddr.sin_port = htons(port);
        return servaddr;
    }

    void set_nodelay(int fd, bool enable) {
        int flag = enable ? 1 : 0;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int)) < 0)
            errno_to_exception();
    }

    Future<FDPtr> connect(Reactor& reactor, std::string addr, int port, std::string bind) {
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);
        set_nodelay(sockfd, true);

        FDPtr fd = reactor.take_fd(sockfd);

        Completer<FDPtr> completer;

        if (!bind.empty()) {
            auto bind_addr = make_addr(bind, 0);
            int res = ::bind(sockfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
            if (res < 0) {
                errno_to_exception();
            }
        }

        auto connect_addr = make_addr(addr, port);
        int res = connect(sockfd, (struct sockaddr *)&connect_addr, sizeof(connect_addr));

        if(res < 0 && errno != EINPROGRESS) {
            errno_to_exception();
        }

        if(res == 0) {
            completer.result(fd);
        } else {
            fd->on_write_ready = [fd, completer, sockfd]() {
                auto refkeep = fd->on_write_ready; // keep everything until exit

                int result;
                socklen_t result_len = sizeof(result);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
                    if(!completer.future().has_result())
                        completer.result_failure(errno_get_exception());
                    return;
                }

                if (result != 0) {
                    errno = result;
                    if(!completer.future().has_result())
                        completer.result_failure(errno_get_exception());
                    return;
                }

                if(!completer.future().has_result()) {
                    completer.result(fd);
                }
            };
            fd->on_error = fd->on_write_ready;
        }

        return completer.future();
    }

    Future<unit> listen(Reactor& reactor, std::string addr, int port, std::function<void(FDPtr)> accept_cb) {
        struct sockaddr_in servaddr = {0};
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);
        FDPtr fd = reactor.take_fd(sockfd);

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(addr.c_str());
        servaddr.sin_port = htons(port);

        int reuseaddr = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0)
            errno_to_exception();

        if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            errno_to_exception();

        if(::listen(sockfd, SOMAXCONN) < 0)
            errno_to_exception();

        LOG("listening on " << addr << ":" << port);

        fd->on_read_ready = [&reactor, fd, sockfd, accept_cb]() {
            while(true) {
                int client = accept(sockfd, 0, 0);
                if(client < 0) {
                    if(errno == EAGAIN || errno == EINPROGRESS)
                        break;
                    else
                        errno_to_exception();
                }
                set_nodelay(client, true);
                accept_cb(reactor.take_fd(client));
            }
        };

        return make_future(unit());
    }
}
