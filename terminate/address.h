#ifndef TERMINATE_IP_H_
#define TERMINATE_IP_H_
#include <stdint.h>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>

struct IpAddress {
    enum { v4, v6 } type;
    union {
        uint32_t addr4;
        uint32_t addr6[4];
    };

    static IpAddress parse4(std::string s) {
        return make4(inet_addr(s.c_str()));
    }

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

    std::string to_string() {
        switch (type) {
        case v4:
            char buf[20];
            sprintf(buf, "%d.%d.%d.%d", addr4 & 0xFF, (addr4 >> 8) & 0xFF, (addr4 >> 16) & 0xFF, (addr4 >> 24) & 0xFF);
            return buf;
        case v6:
        default:
            abort();
        }
    }
};

#endif
