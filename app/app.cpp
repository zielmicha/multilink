#include "multilink.h"
#include <iostream>
#include <memory>
#include "rpc.h"
#include "ioutil.h"
#include "throttled.h"
#include "packet_stream_util.h"

class Server {
    Reactor& reactor;
    std::unordered_map<int, Stream*> unused_stream_fds;
    std::unordered_map<int, std::shared_ptr<Multilink::Multilink> > multilinks;

    // TODO: move from raw pointers to shared_ptrs
    std::vector<std::shared_ptr<void>> keep_pointers;

    template <typename F>
    F* keep_from_deletion(std::shared_ptr<F> s) {
        keep_pointers.push_back(s);
        return &(*s);
    }

    Stream* take_fd(int num) {
        assert(unused_stream_fds.find(num) != unused_stream_fds.end());
        auto fd = unused_stream_fds[num];
        unused_stream_fds.erase(unused_stream_fds.find(num));
        return fd;
    }

public:
    Server(Reactor& reactor):
        reactor(reactor) {}

    void callback(std::shared_ptr<RPCStream> stream,
                  Json message) {
        std::string type = message["type"].string_value();
        std::cerr << "recv " << message.dump() << std::endl;

        if(type == "provide-stream") {
            int num = message["num"].int_value();
            FD* fd = stream->abandon();
            unused_stream_fds[num] = fd;
            ioutil::write(fd, ByteString::copy_from("ok\n"));
        } else if(type == "multilink") {
            int num = message["num"].int_value();
            int stream_fd = message["stream_fd"].int_value();
            multilinks[num] = std::make_shared<Multilink::Multilink>(reactor);
            auto target = LengthPacketStream::create(reactor, take_fd(stream_fd));
            pipe(reactor, multilinks[num], target, Multilink::MULTILINK_MTU);
            pipe(reactor, target, multilinks[num], Multilink::MULTILINK_MTU);
        } else if(type == "add-link") {
            int num = message["num"].int_value();
            int stream_fd = message["stream_fd"].int_value();
            multilinks[num]->add_link(take_fd(stream_fd), message["name"].string_value());
        } else if(type == "limit-stream") {
            int stream_fd = message["stream_fd"].int_value();
            Stream* s = take_fd(stream_fd);
            int delay = message["delay"].int_value();
            if(delay != 0)
                s = new DelayedStream(reactor, s,
                                      message["buffsize"].int_value(),
                                      delay);
            s = new ThrottledStream(reactor, s, message["mbps"].number_value());
            s->set_on_read_ready([](){});
            s->set_on_write_ready([](){});
            unused_stream_fds[stream_fd] = s;
        } else {
            std::cerr << "bad type " << type << std::endl;
        }
    }
};

int main() {
    Reactor reactor;

    Server server {reactor};
    auto callback = [&](std::shared_ptr<RPCStream> stream,
                        Json message) {
        server.callback(stream, message);
    };
    auto rpcserver = RPCServer::create(reactor, "app.sock",
                                       callback);

    reactor.run();
}
