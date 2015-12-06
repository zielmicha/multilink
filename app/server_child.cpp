#define LOGGER_NAME "server_child"
#include "libreactor/logging.h"
#include "libreactor/ioutil.h"
#include "app/rpc.h"
#include "multilink/multilink.h"
#include "multilink/transport.h"
#include "multilink/transport_targets.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using std::string;
using json11::Json;

struct Server {
    std::shared_ptr<Multilink::Multilink> multilink;
    Reactor& reactor;

    Server(Reactor& reactor): reactor(reactor) {

    }

    void setup_connect_target(string host, int port) {
        multilink = std::make_shared<Multilink::Multilink>(reactor);

        TargetCreator target_creator = create_connecting_tcp_target_creator(reactor, host, port);
        Transport::create(reactor, multilink,
                          target_creator, Multilink::MULTILINK_MTU);
    }

    void callback(std::shared_ptr<RPCStream> stream,
                  Json message) {
        std::string cmd_name = message["type"].string_value();

        if (cmd_name == "add-link") {
            FDPtr fd = stream->abandon();
            ioutil::write(fd, ByteString::copy_from("ok\n"));
            multilink->add_link(fd, message["name"].string_value());
        } else {
            ERROR("bad command: " << cmd_name);
        }
    }
};

int main(int argc, char** argv) {
    setup_crash_handlers();

    po::options_description desc ("Start Multilink server child");
    desc.add_options()
        ("help", "produce help message")
        ("sock-fd",
         po::value<int>(), "FD of socket to start RPC server on")
        ("target-port",
         po::value<int>(), "target port (if using connect target)")
        ("target-host",
         po::value<string>()->default_value("127.0.0.1"), "target host")
        ("dry-run", "only verify that options make sense");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    if (vm.count("dry-run")) return 0;

    Reactor reactor;
    Server server {reactor};

    auto callback = [&](std::shared_ptr<RPCStream> stream,
                        Json message) {
        server.callback(stream, message);
    };

    if (vm.count("target-port")) {
        server.setup_connect_target(vm["target-host"].as<string>(),
                                    vm["target-port"].as<int>());
    }

    auto rpcserver = RPCServer::create(reactor, vm["sock-fd"].as<int>(), callback);
    reactor.run();
}
