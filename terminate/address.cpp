#include "terminate/address.h"
#include "libreactor/misc.h"
#include "libreactor/common.h"
#define LOGGER_NAME "address"
#include "libreactor/logging.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::vector<IpAddress> IpAddress::get_addresses() {
    std::vector<IpAddress> result;

    struct ifreq ifreqs[20];
    struct ifconf ic;

    ic.ifc_len = sizeof ifreqs;
    ic.ifc_req = ifreqs;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        errno_to_exception();
    }

    if (ioctl(sock, SIOCGIFCONF, &ic) < 0) {
        errno_to_exception();
    }


    for (int i=0; i < ic.ifc_len / sizeof(struct ifreq); i ++) {
        std::string iface_name = ifreqs[i].ifr_name;
        bool bad = false;
        for (std::string bad_prefix : {"lo", "tun", "lxcbr", "virbr"}) {
            if (iface_name.substr(0, bad_prefix.size()) == bad_prefix)
                bad = true;
        }
        if (bad) continue;
        struct sockaddr* addr = &ifreqs[i].ifr_addr;
        if (addr->sa_family == AF_INET) {
            uint32_t a = ((struct sockaddr_in*)addr)->sin_addr.s_addr;
            result.push_back(IpAddress::make4(a));
        }
    }
    return result;
}
