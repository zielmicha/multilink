#include "tcp.h"
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace TCP {
    Future<FD*> connect(Reactor& reactor, std::string addr, int port) {
        struct sockaddr_in servaddr = {0};
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);
        FD* fd = &reactor.take_fd(sockfd);

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(addr.c_str());
        servaddr.sin_port = htons(port);

        Future<FD*> future;
        int res = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

        if(res < 0 && errno != EINPROGRESS) {
            errno_to_exception();
        }

        if(res == 0) {
            future = make_future(fd);
        } else {
            fd->on_write_ready = [fd, future, sockfd]() {
                int result;
                socklen_t result_len = sizeof(result);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
                    errno_to_exception();
                }

                if (result != 0) {
                    errno = result;
                    errno_to_exception();
                }

                if(!future.has_result())
                    future.result(fd);
            };
            fd->on_error = fd->on_write_ready;
        }

        return future;
    }
}
