#define LOGGER_NAME "multilink_client"
#include <boost/program_options.hpp>
#include "logging.h"
#include "misc.h"
#include "multilink.h"
#include "transport_targets.h"
#include "transport.h"

namespace po = boost::program_options;
using std::string;

struct Client {
    Reactor reactor;
    std::shared_ptr<Multilink::Multilink> ml;
    string addr;
    int port;

    Client(string addr, int port): addr(addr), port(port) {
        ml = std::make_shared<Multilink::Multilink>(reactor);
    }

    void create_listening_target(int port) {
        auto target_creator = unknown_stream_target_creator();
        std::shared_ptr<Transport> target = Transport::create(reactor, ml,
                                                              target_creator, Multilink::MULTILINK_MTU);

        create_listening_tcp_target_creator(reactor, target, "127.0.0.1", port);
    }

    void add_bind_address(string addr) {

    }
};

int main(int argc, char** argv) {
    po::options_description desc ("Start Multilink client");
    desc.add_options()
        ("help", "produce help message")
        ("listen-port,P",
         po::value<int>()->default_value(5500), "port to listen to")
        ("connect-address,c",
         po::value<string>()->default_value("127.0.0.1"), "address to connect to")
        ("connect-port,p",
         po::value<int>()->default_value(4500), "port to connect to")
        ("identity,i",
         po::value<string>()->default_value("hello"), "PSK identity")
        ("psk,p",
         po::value<string>()->default_value("6d41adad3518a0e8ba0362bd9ef03244"), "PSK (hex encoded)")
        ("static,s",
         po::value<std::vector<string> >(), "bind on these addresses when creating connections");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || vm["static"].empty()) {
        std::cout << desc << "\n";
        return 0;
    }

    Client client (vm["connect-address"].as<string>(), vm["connect-port"].as<int>());
    client.create_listening_target(vm["listen-port"].as<int>());
    for (string s : vm["static"].as<std::vector<string> >()) {
        client.add_bind_address(s);
    }
}
