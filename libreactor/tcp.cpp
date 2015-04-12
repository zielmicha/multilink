#include "tcp.h"
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define LOGGER_NAME "tcp"
#include "logging.h"

namespace TCP {
    Future<FD*> connect(Reactor& reactor, std::string addr, int port) {
        struct sockaddr_in servaddr = {0};
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);
        FD* fd = &reactor.take_fd(sockfd);

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(addr.c_str());
        servaddr.sin_port = htons(port);

        Completer<FD*> completer;
        int res = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

        if(res < 0 && errno != EINPROGRESS) {
            errno_to_exception();
        }

        if(res == 0) {
            completer.result(fd);
        } else {
            fd->on_write_ready = [fd, completer, sockfd]() {
                int result;
                socklen_t result_len = sizeof(result);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
                    completer.result_failure(errno_get_exception());
                    return;
                }

                if (result != 0) {
                    errno = result;
                    completer.result_failure(errno_get_exception());
                    return;
                }

                if(!completer.future().has_result()) {
                    // reactor.later([fd]() { fd->on_write_ready(); })
                    completer.result(fd);
                }
            };
            fd->on_error = fd->on_write_ready;
        }

        return completer.future();
    }

    Future<unit> listen(Reactor& reactor, std::string addr, int port, std::function<void(FD*)> accept_cb) {
        struct sockaddr_in servaddr = {0};
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);
        FD* fd = &reactor.take_fd(sockfd);

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

        fd->on_read_ready = [&reactor, fd, sockfd, accept_cb]() {
            while(true) {
                int client = accept(sockfd, 0, 0);
                if(client < 0) {
                    if(errno == EAGAIN || errno == EINPROGRESS)
                        break;
                    else
                        errno_to_exception();
                }
                accept_cb(&reactor.take_fd(client));
            }
        };

        return make_future(unit());
    }
}
