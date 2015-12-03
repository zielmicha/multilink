#include "multilink/multilink.h"
#include "multilink/transport.h"
#include "multilink/transport_targets.h"
#include <iostream>
#include <memory>
#include "app/rpc.h"
#include "libreactor/ioutil.h"
#include "libreactor/throttled.h"
#include "libreactor/packet_stream_util.h"
#include "libreactor/logging.h"

class Server {
    Reactor& reactor;
    std::unordered_map<int, StreamPtr> unused_stream_fds;
    std::unordered_map<int, std::shared_ptr<Multilink::Multilink> > multilinks;
    std::unordered_map<int, std::shared_ptr<Transport> > transports;

    StreamPtr take_fd(int num) {
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

        std::unordered_map<std::string, std::function<void()>> ops;

        ops["provide-stream"] = [&]() {
            int num = message["num"].int_value();
            FDPtr fd = stream->abandon();
            unused_stream_fds[num] = fd;
            ioutil::write(fd, ByteString::copy_from("ok\n"));
        };

        ops["pass-stream"] = [&]() {
            int num = message["num"].int_value();
            stream->recv_fd().then<unit>([=](int fd) -> unit {
                unused_stream_fds[num] = reactor.take_fd(fd);
                ioutil::write(stream->abandon(), ByteString::copy_from("ok\n"));
                return {};
            });
        };

        ops["multilink"] = [&]() {
            int num = message["num"].int_value();
            int stream_fd = message["stream_fd"].int_value();
            multilinks[num] = std::make_shared<Multilink::Multilink>(reactor);
            std::shared_ptr<PacketStream> target;
            if(!message["free"].bool_value()) {
                target = LengthPacketStream::create(reactor, take_fd(stream_fd));
            } else {
                target = FreePacketStream::create(reactor, take_fd(stream_fd),
                                                  Multilink::MULTILINK_MTU);
            }
            pipe(reactor, multilinks[num], target, Multilink::MULTILINK_MTU);
            pipe(reactor, target, multilinks[num], Multilink::MULTILINK_MTU);
        };

        ops["add-link"] = [&]() {
            int num = message["num"].int_value();
            int stream_fd = message["stream_fd"].int_value();
            multilinks[num]->add_link(take_fd(stream_fd), message["name"].string_value());
        };

        ops["limit-stream"] = [&]() {
            int stream_fd = message["stream_fd"].int_value();
            StreamPtr s = take_fd(stream_fd);
            int delay = message["delay"].int_value();
            if(delay != 0)
                s = std::make_shared<DelayedStream>(reactor, s,
                                                    message["buffsize"].int_value(),
                                                    delay);
            s = std::make_shared<ThrottledStream>(reactor, s, message["mbps"].number_value());
            s->set_on_read_ready([](){});
            s->set_on_write_ready([](){});
            unused_stream_fds[stream_fd] = s;
        };

        ops["multilink-client-server"] = [&]() {
            int multilink_num = message["num"].int_value();
            bool is_server = message["is_server"].bool_value();

            std::string host = message["host"].string_value();
            int port = message["port"].int_value();

            multilinks[multilink_num] = std::make_shared<Multilink::Multilink>(reactor);

            TargetCreator target_creator;

            if(is_server) {
                target_creator = create_connecting_tcp_target_creator(reactor, host, port);
            } else {
                target_creator = unknown_stream_target_creator();
            }

            std::shared_ptr<Transport> target = Transport::create(reactor,
                                                                  multilinks[multilink_num],
                                                                  target_creator, Multilink::MULTILINK_MTU);

            if(!is_server) {
                create_listening_tcp_target_creator(reactor, target, host, port);
            }
        };

        auto method = ops.find(type);
        if(method == ops.end())
            std::cerr << "bad call " << type << std::endl;
        else
            method->second();
    }
};

int main(int argc, char** args) {
    setup_crash_handlers();

    Reactor reactor;

    Server server {reactor};
    auto callback = [&](std::shared_ptr<RPCStream> stream,
                        Json message) {
        server.callback(stream, message);
    };

    const char* path = argc == 2 ? args[1] : "app.sock";
    LOG("listening on " << path);

    std::shared_ptr<RPCServer> rpcserver;
    if (path[0] == '&') {
        rpcserver = RPCServer::create(reactor, atoi(path + 1), callback);
    } else {
        rpcserver = RPCServer::create(reactor, std::string(path), callback);
    }

    reactor.run();
}
