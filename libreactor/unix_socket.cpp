#include "unix_socket.h"
#include "common.h"
#include "logging.h"
#include "misc.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/types.h>

namespace UnixSocket {
    void listen(Reactor& reactor, std::string path, std::function<void(FD*)> accept_cb) {
        unlink(path.c_str());

        struct sockaddr_un servaddr = {0};
        int sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        setnonblocking(sockfd);

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sun_family = AF_UNIX;
        strncpy(servaddr.sun_path, path.c_str(), sizeof(servaddr.sun_path)-1);

        if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            errno_to_exception();

        if(::listen(sockfd, SOMAXCONN) < 0)
            errno_to_exception();

        return listen_on_fd(reactor, sockfd, accept_cb);
    }

    void listen_on_fd(Reactor& reactor, int sockfd, std::function<void(FD*)> accept_cb) {
        FD* fd = &reactor.take_fd(sockfd);

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

    int do_recv_fd(int fileno);

    Future<int> recv_fd(Reactor& reactor, FD* socket) {
        Completer<int> completer;
        auto on_read = [&reactor, socket, completer]() {
            int fileno = socket->fileno();
            int fd = do_recv_fd(fileno);
            if(fd != -1) {
                auto completerPtr = completer;
                socket->set_on_read_ready(nothing);
                completerPtr.result(fd);
                return;
            }
        };

        socket->set_on_read_ready(on_read);
        on_read();

        return completer.future();
    }

    union fdmsg {
        struct cmsghdr h;
        char buf[CMSG_SPACE(sizeof(int))];
    };

    int do_recv_fd(int sock_fd) {
        struct iovec iov[1];
        struct msghdr msg;

        int fake;
        iov[0].iov_base = &fake;
        iov[0].iov_len  = 1;

        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;

        union fdmsg cmsg;
		struct cmsghdr* h;

		msg.msg_control = cmsg.buf;
		msg.msg_controllen = sizeof(union fdmsg);
		msg.msg_flags = 0;

		h = CMSG_FIRSTHDR(&msg);
		h->cmsg_len = CMSG_LEN(sizeof(int));
		h->cmsg_level = SOL_SOCKET;
		h->cmsg_type = SCM_RIGHTS;
		*((int*)CMSG_DATA(h)) = -1;

        int count;

		if ((count = recvmsg(sock_fd, &msg, 0)) < 0) {
			return -1;
		} else {
			h = CMSG_FIRSTHDR(&msg);
			if (h == NULL
			    || h->cmsg_len != CMSG_LEN(sizeof(int))
			    || h->cmsg_level != SOL_SOCKET
			    || h->cmsg_type != SCM_RIGHTS ) {
				LOG("failed to recv fd in an unexpected way");
                return -1;
			} else {
				return *((int*)CMSG_DATA(h));
			}
		}
    }
}
