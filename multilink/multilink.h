#ifndef MULTILINK_H
#define MULTILINK_H
#include <cstdint>
#include <string>
#include <iostream>
#include "reactor.h"
#include "multilink_stats.h"

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
        std::string name;

    public:
        Link(const Link& l) = delete;

        void display(std::ostream& stream) const;

        friend std::ostream& operator<<(std::ostream& stream, const Link& link) {
            link.display(stream);
            return stream;
        }
    };

    class Multilink {
    private:
        friend class Link;
    public:
        Multilink();
        Multilink(const Multilink& link) = delete;

        Link& add_link(Stream* stream, std::string name = "default");

        std::function<void()> on_send_ready;
        std::function<void()> on_recv_ready;

        void send(ChannelInfo channel, const char* buff, size_t length);
        size_t recv(ChannelInfo& channel, const char* buff, size_t length);
    };
}
#endif
