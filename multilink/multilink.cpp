#include "multilink.h"

namespace Multilink {
    Multilink::Multilink(Reactor& reactor): reactor(reactor) {

    }

    void Multilink::send(ChannelInfo channel, const Buffer data) {

    }

    optional<Buffer> Multilink::recv(ChannelInfo& channel,
                 Buffer data) {
        return optional<Buffer>();
    }

    Link& Multilink::add_link(Stream* stream, std::string name) {
        links.emplace_back(new Link(reactor, stream));
        Link& link = *links.back();
        link.name = name;
        return link;
    }
}
