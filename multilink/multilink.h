#ifndef MULTILINK_H
#define MULTILINK_H
#include <cstdint>
#include <string>
#include "reactor.h"

namespace Multilink {
    struct ChannelInfo {
        uint8_t type;
        uint64_t id;

        enum {
            TYPE_COMMON = 0x01,
            TYPE_TCP = 0x02,
            TYPE_RAW = 0x81,
        };
    };

    class Link {
    private:
        friend class Multilink;
        Link();
        Stream* stream;

    public:
        Link(const Link& l) = delete;
    };

    class Multilink {
    private:
        friend class Link;
    public:
        Multilink();
        Multilink(const Multilink& link) = delete;

        Link& add_link(Stream* stream, std::string doc = "");

        std::function<void()> on_send_ready;
        std::function<void()> on_recv_ready;

        void send(ChannelInfo channel, const char* buff, size_t length);
        size_t recv(ChannelInfo& channel, const char* buff, size_t length);
    };
}
#endif
