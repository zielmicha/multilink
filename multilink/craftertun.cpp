#include "common.h"
#include "craftertun.h"

#define LOGGER_NAME "tun"
#include "logging.h"

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

Tun::Tun(Reactor& r, std::string name): reactor(r) {
    struct ifreq ifr;
    int fd;

    if((fd = open("/dev/net/tun", O_RDWR)) < 0)
        errno_to_exception();

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0)
        errno_to_exception();

    this->name = ifr.ifr_name;
    LOG("Created tun interface " << this->name << ".");

    this->fd = &reactor.take_fd(fd);
    this->fd->on_read_ready = [this]() {
        on_read();
    };

    this->fd->on_write_ready = []() {
        LOG("can write");
    };
}

void Tun::on_read() {
    StackBuffer<MTU> buffer;
    while(true) {
        Buffer data = fd->read(buffer);
        if(data.size == 0) break;
        Crafter::Packet packet;
        const int offset = 4;
        packet.PacketFromIP((unsigned char*)(data.data + offset), data.size - offset);
        on_recv(packet);
    }
}
