#include "unix_socket.h"
#include "common.h"
#include <sys/socket.h>
#include <sys/un.h>

namespace UnixSocket {
    void listen(Reactor& reactor, std::string path, std::function<void(FD*)> accept_cb) {
        struct sockaddr_un servaddr = {0};
        int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);

        FD* fd = &reactor.take_fd(sockfd);

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sun_family = AF_UNIX;
        strncpy(servaddr.sun_path, path.c_str(), sizeof(servaddr.sun_path)-1);

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
    }
}
