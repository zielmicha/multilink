#define LOGGER_NAME "multilink_client"
#include <boost/program_options.hpp>
#include <unistd.h>
#include <fstream>
#include <set>
#include "libreactor/logging.h"
#include "libreactor/misc.h"
#include "multilink/multilink.h"
#include "libreactor/bytestring.h"
#include "multilink/transport_targets.h"
#include "multilink/transport.h"
#include "libreactor/tls.h"
#include "libreactor/ioutil.h"
#include "libreactor/process.h"
#include "libreactor/signals.h"
#include "terminate/terminate.h"

namespace po = boost::program_options;
using std::string;

const string PID_FILE = "/var/run/multilink-iface-notify.pid";

struct Client {
    Reactor reactor;
    std::shared_ptr<Multilink::Multilink> ml;
    string addr;
    int port;
    ByteString client_id;
    string identity, psk;
    string instance_id;
    std::function<void()> on_init;
    std::multiset<string> running_links;
    Timer timer;

    Client(string addr, int port, string identity, string psk):
        addr(addr), port(port), identity(identity), psk(psk), timer(reactor) {
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
        std::shared_ptr<Terminator> terminator = Terminator::create(reactor, false);
        std::shared_ptr<Tun> tun = Tun::create(reactor, tun_name);
        terminator->set_tun(tun, "10.77.0.1");

        Popen(reactor, {"ip", "route", "replace", "default", "dev", tun_name})
            .check_call().wait(reactor);

        TargetCreator target_creator = std::bind(&Terminator::create_target, terminator,
                                                 std::placeholders::_1);

        std::shared_ptr<Transport> target = Transport::create(reactor, ml,
                                                              target_creator, TRANSPORT_MTU);
        terminator->set_transport(target);
    }

    void add_bind_address(string bind_addr) {
        if (running_links.find(bind_addr) != running_links.end())
            return;

        LOG ("add bind address: " << bind_addr);
        running_links.insert(bind_addr);
        TCP::connect(reactor, this->addr, this->port, bind_addr).then([=](StreamPtr stream) {
            auto tls_stream = TlsStream::create(reactor, stream);
            tls_stream->handshake_as_client();
            tls_stream->set_psk_client_callback([this](const char* hint) {
                return TlsStream::IdentityAndPsk
                    (ByteString::copy_from(identity), ByteString::copy_from(psk));
            });
            tls_stream->set_cipher_list("PSK-AES256-CBC-SHA");

            ByteString welcome (128);
            welcome.as_buffer().set_zero();
            welcome.as_buffer().slice(0, 16).copy_from(client_id);
            welcome.as_buffer().slice(16).copy_from(Buffer(bind_addr));

            return ioutil::write(tls_stream, welcome).then([=](unit) {
                return ioutil::read(tls_stream, 16);
            }).then([=](ByteString instance_id_buff) {
                string instance_id = instance_id_buff;
                if (this->instance_id == "")
                    this->instance_id = instance_id;
                LOG("connected to instance " << hex_encode(instance_id));
                if (this->instance_id == instance_id)
                    ml->add_link(tls_stream, bind_addr,
                                 std::bind(&Client::link_closed, this, bind_addr));
                else
                    abort_instance();

                return unit();
            });
        }).on_success_or_failure([=](unit) {
            LOG("connection on " << bind_addr << " established");
        }, [=](std::unique_ptr<std::exception> ex) {
            LOG("connection on " << bind_addr << " failed");
            link_closed(bind_addr);
        });
    }

    void link_closed(string bind_addr) {
        if (running_links.find(bind_addr) != running_links.end())
            running_links.erase(running_links.find(bind_addr));
        timer.once(2000 * 1000, std::bind(&Client::add_bind_address, this, bind_addr));
    }

    void abort_instance() {
        // new server instance appeared, restart everything
        LOG("server restarted!");
        instance_id = "";
        // FIXME: we need to ensure that there are no outstanding scheduled functions
        on_init();
    }

    void enable_nm_integration() {
        Signals::register_signal_handler(SIGUSR1, std::bind(&Client::reload_nm, this));

        std::ofstream pid_file;
        pid_file.open(PID_FILE);
        pid_file << getpid() << std::endl;
        pid_file.close();

        Popen(reactor, "./scripts/network-manager-setup.sh").check_call().wait(reactor);
        reload_nm();
    }

    void reload_nm() {
        LOG("reloading interface list");

        std::vector<string> missing;
        for (IpAddress addr_obj : IpAddress::get_addresses()) {
            string addr = addr_obj.to_string();
            if (running_links.find(addr) == running_links.end()) {
                missing.push_back(addr);
            }
        }

        for (string addr : missing)
            add_bind_address(addr);
    }

    void run() {
        on_init();
        reactor.run();
    }
};

int main(int argc, char** argv) {
    po::options_description desc ("Start Multilink client");
    desc.add_options()
        ("help", "produce help message")
        ("listen-port,P",
         po::value<int>(), "port to listen to (if not using tun transport)")
        ("tun-name",
         po::value<string>(), "tun name (for tun transport)")
        ("connect-address,c",
         po::value<string>()->default_value("127.0.0.1"), "address to connect to")
        ("connect-port,p",
         po::value<int>()->default_value(4500), "port to connect to")
        ("identity",
         po::value<string>()->default_value("hello"), "PSK identity")
        ("psk",
         po::value<string>()->default_value("6d41adad3518a0e8ba0362bd9ef03244"), "PSK (hex encoded)")
        ("static,s",
         po::value<std::vector<string> >(), "bind on these addresses when creating connections")
        ("network-manager-integration,n", "enable Network Manager integration");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    bool nm_integration = vm.count("network-manager-integration") != 0;

    if (vm.count("help") || (vm["static"].empty() && !nm_integration) || !(vm["listen-port"].empty() ^ vm["tun-name"].empty())) {
        std::cout << desc << "\n";
        return 0;
    }

    TlsStream::init();
    Process::init();

    Client client (vm["connect-address"].as<string>(), vm["connect-port"].as<int>(),
                   vm["identity"].as<string>(),
                   hex_decode(vm["psk"].as<string>()));

    client.on_init = [&]() {
        if (!vm["listen-port"].empty()) {
            client.create_listening_target(vm["listen-port"].as<int>());
        } else {
            client.create_terminate_target(vm["tun-name"].as<string>());
        }

        if (nm_integration) {
            client.enable_nm_integration();
        }

        if (vm.count("static")) {
            for (string s : vm["static"].as<std::vector<string> >()) {
                client.add_bind_address(s);
            }
        }
    };

    client.run();
}
