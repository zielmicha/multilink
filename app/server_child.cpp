#define LOGGER_NAME "server_child"
#include "libreactor/logging.h"
#include "libreactor/ioutil.h"
#include "libreactor/process.h"
#include "app/rpc.h"
#include "multilink/multilink.h"
#include "multilink/transport.h"
#include "multilink/transport_targets.h"
#include "terminate/terminate.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using std::string;
using json11::Json;

struct Server {
    std::shared_ptr<Multilink::Multilink> multilink;
    Reactor& reactor;
    std::shared_ptr<Terminator> terminator;

    Server(Reactor& reactor): reactor(reactor) {

    }

    void setup_connect_target(string host, int port) {
        multilink = std::make_shared<Multilink::Multilink>(reactor);

        TargetCreator target_creator = create_connecting_tcp_target_creator(reactor, host, port);
        Transport::create(reactor, multilink,
                          target_creator, Multilink::MULTILINK_MTU);
    }

    void setup_terminate_target(string bind_address) {
        multilink = std::make_shared<Multilink::Multilink>(reactor);

        terminator = Terminator::create(reactor, true);
        terminator->bind_address = bind_address;

        TargetCreator target_creator = std::bind(&Terminator::create_target, terminator,
                                                 std::placeholders::_1);

        auto transport = Transport::create(reactor, multilink, target_creator,
                                           Multilink::MULTILINK_MTU);

        terminator->set_transport(transport);
    }

    void setup_tun(string tun_name) {
        TunPtr tun = Tun::create(reactor, tun_name);
        terminator->set_tun(tun, "10.77.0.2");
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
        ("connect-bind",
         po::value<string>()->default_value("0.0.0.0"), "bind to this host when creating outbound TCP connections")
        ("tun-prefix",
         po::value<string>()->default_value("mlc"), "TUN device name prefix (if using terminate target)")
        ("dry-run", "only verify that options make sense");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    if (vm.count("target-port") && vm.count("tun-prefix")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("dry-run")) return 0;

    Process::init();

    Reactor reactor;
    Server server {reactor};

    auto callback = [&](std::shared_ptr<RPCStream> stream,
                        Json message) {
        server.callback(stream, message);
    };

    if (vm.count("target-port")) {
        server.setup_connect_target(vm["target-host"].as<string>(),
                                    vm["target-port"].as<int>());
    } else {
        server.setup_terminate_target(vm["connect-bind"].as<string>());
    }

    if (vm.count("tun-prefix")) {
        string prefix = vm["tun-prefix"].as<string>();
        string name = prefix + random_hex_string(15 - prefix.size());
        server.setup_tun(name);
    }

    auto rpcserver = RPCServer::create(reactor, vm["sock-fd"].as<int>(), callback);
    reactor.run();
}
