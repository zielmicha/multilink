#include "libreactor/common.h"
#include "terminate/tun.h"

#define LOGGER_NAME "tun"
#include "libreactor/logging.h"

#include <sys/ioctl.h>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>

const int MTU = 4096;

Tun::Tun(Reactor& r): reactor(r), buffer(MTU) {}

TunPtr Tun::create(Reactor& reactor, std::string name) {
    TunPtr self = std::make_shared<Tun>(reactor);

    struct ifreq ifr;
    int fd;

    if((fd = open("/dev/net/tun", O_RDWR)) < 0)
        errno_to_exception();

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0)
        errno_to_exception();

    self->name = ifr.ifr_name;
    LOG("Created tun interface " << self->name << ".");

    self->fd = reactor.take_fd(fd);
    self->fd->on_read_ready = std::bind(&Tun::transport_ready_ready, self);

    self->fd->on_write_ready = []() {};

    return self;
}

void Tun::transport_ready_ready() {
    on_recv_ready();
}

optional<Buffer> Tun::recv() {
    Buffer data = fd->read(buffer.as_buffer());
    if(data.size == 0) return optional<Buffer>();
    return data;
}

bool Tun::is_send_ready() {
    return true;
}

void Tun::send(const Buffer data) {
    size_t s = fd->write(data);
    if (s != data.size)
        ERROR("partial write to tun (" << s << " out of " << data.size << ")");
}

void Tun::close() {
    fd->close();
}
