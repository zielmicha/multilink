#ifndef TERMINATE_IP_H_
#define TERMINATE_IP_H_
#include <stdint.h>

struct IpAddress {
    enum { v4, v6 } type;
    union {
        uint32_t addr4;
        uint32_t addr6[4];
    };

    static IpAddress make4(uint32_t a) {
        IpAddress ret;
        ret.type = v4;
        ret.addr4 = a;
        return ret;
    }

    static IpAddress make6(uint32_t* a) {
        IpAddress ret;
        ret.type = v4;
        memcpy(ret.addr6, a, sizeof(ret.addr6));
        return ret;
    }
};

#endif
