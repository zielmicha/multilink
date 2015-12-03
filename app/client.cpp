#define LOGGER_NAME "multilink_client"
#include <boost/program_options.hpp>
#include "libreactor/logging.h"
#include "libreactor/misc.h"
#include "multilink/multilink.h"
#include "libreactor/bytestring.h"
#include "multilink/transport_targets.h"
#include "multilink/transport.h"
#include "libreactor/tls.h"
#include "libreactor/ioutil.h"
#include "terminate/terminate.h"

namespace po = boost::program_options;
using std::string;

struct Client {
    Reactor reactor;
    std::shared_ptr<Multilink::Multilink> ml;
    string addr;
    int port;
    ByteString client_id;
    string identity, psk;

    Client(string addr, int port, string identity, string psk):
        addr(addr), port(port), identity(identity), psk(psk) {
        ml = std::make_shared<Multilink::Multilink>(reactor);
        client_id = ByteString::copy_from(hex_decode(random_hex_string(32)));
    }

    const size_t TRANSPORT_MTU = (size_t) Multilink::MULTILINK_MTU - Transport::HEADER_SIZE;

    void create_listening_target(int port) {
        auto target_creator = unknown_stream_target_creator();
        std::shared_ptr<Transport> target = Transport::create(reactor, ml,
                                                              target_creator, TRANSPORT_MTU);

        create_listening_tcp_target_creator(reactor, target, "127.0.0.1", port);
    }

    void create_terminate_target(string tun_name) {
        auto target_creator = unknown_stream_target_creator();
        std::shared_ptr<Transport> target = Transport::create(reactor, ml,
                                                              target_creator, TRANSPORT_MTU);

        create_listening_tcp_target_creator(reactor, target, "127.0.0.1", port);
    }

    void add_bind_address(string bind_addr) {
        TCP::connect(reactor, this->addr, this->port, bind_addr).then([=](StreamPtr stream) {
            auto tls_stream = new TlsStream(reactor, stream);
            tls_stream->handshake_as_client();
            tls_stream->set_psk_client_callback([this](const char* hint) {
                return TlsStream::IdentityAndPsk
                    (ByteString::copy_from(identity), ByteString::copy_from(psk));
            });
            tls_stream->set_cipher_list("PSK-AES256-CBC-SHA");
            return ioutil::write(tls_stream, client_id).then([=](unit) {
                ml->add_link(tls_stream, bind_addr);
                return unit();
            });
        }).on_success_or_failure([=](unit) {
            LOG("connection on " << bind_addr << " established");
        }, [=](std::unique_ptr<std::exception> ex) {
            LOG("connection on " << bind_addr << " failed");
        });
    }

    void run() {
        reactor.run();
    }
};

int main(int argc, char** argv) {
    po::options_description desc ("Start Multilink client");
    desc.add_options()
        ("help", "produce help message")
        ("listen-port,P",
         po::value<int>(), "port to listen to (if not using tun transport)")
        ("tun-name,t",
         po::value<string>(), "tun name (for tun transport)")
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

    if (vm.count("help") || vm["static"].empty() || (vm["listen-port"].empty() ^ vm["tun-name"].empty())) {
        std::cout << desc << "\n";
        return 0;
    }

    TlsStream::init();

    Client client (vm["connect-address"].as<string>(), vm["connect-port"].as<int>(),
                   vm["identity"].as<string>(),
                   hex_decode(vm["psk"].as<string>()));

    if (!vm["listen-port"].empty()) {
        client.create_listening_target(vm["listen-port"].as<int>());
    } else {
        client.create_terminate_target(vm["tun-name"].as<string>());
    }

    for (string s : vm["static"].as<std::vector<string> >()) {
        client.add_bind_address(s);
    }

    client.run();
}
