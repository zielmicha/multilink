#define LOGGER_NAME "multilink_server"
#include "libreactor/logging.h"
#include "libreactor/tcp.h"
#include "libreactor/tls.h"
#include "libreactor/ioutil.h"
#include "libreactor/misc.h"
#include "libreactor/process.h"
#include "libreactor/unix_socket.h"
#include "libreactor/ioutil.h"
#include <boost/program_options.hpp>
#include <fstream>
#include "json11.hpp"
#include "multilink/multilink.h"

namespace po = boost::program_options;

using std::string;
using json11::Json;

struct ProcessHandler {
    Reactor& reactor;
    string path;
    ProcessPtr process;

    PacketStreamPtr main_stream;
    int stream_counter = 0;
    int multilink_num = 0;

    std::function<void()> on_close;

    ProcessHandler(Reactor& reactor, std::function<void()> on_close, string target_host, int target_port):
        reactor(reactor), on_close(on_close) {
        path = "/tmp/.ml_" + random_hex_string(32);

        FDPtr server_socket = UnixSocket::bind(reactor, path);
        server_socket->set_close_on_exec(false);
        process = Popen(reactor, {"./build/app", "&" + std::to_string(server_socket->fileno())}).exec();
        server_socket->close();

        main_stream = LengthPacketStream::create(reactor, UnixSocket::connect(reactor, path));
        multilink_num = 0;

        send_message(main_stream, Json::object {
                {"type", "multilink-client-server"},
                {"is_server", true},
                {"host", target_host},
                {"port", target_port},
                {"num", multilink_num}
            }).wait(reactor);
    }

    ProcessHandler(const ProcessHandler& other) = delete;

    Future<unit> send_message(PacketStreamPtr stream, Json value) {
        return ioutil::send(stream, ByteString::copy_from(value.dump()));
    }

    Future<int> provide_stream(StreamPtr stream) {
        auto raw_stream = UnixSocket::connect(reactor, path);
        auto control_stream = LengthPacketStream::create(reactor, raw_stream);
        int stream_num = stream_counter ++;

        send_message(control_stream, Json::object {
            { "type", "provide-stream" },
            { "num", stream_num }
        }).ignore();

        return ioutil::read(raw_stream, 3).then([=](ByteString s) -> int {
            if (string(s) != "ok\n") {
                LOG("unexpected response from app: " << s);
                abort();
            }
            pipe_both(reactor, stream, raw_stream, Multilink::LINK_MTU);
            return stream_num;
        });
    }

    void add_link(StreamPtr stream, string name) {
        provide_stream(stream).then([=](int stream_num) {
            return send_message(main_stream, Json::object {
                { "type", "add-link" },
                { "stream_fd", stream_num },
                { "num", multilink_num }
            });
        }).then([=](unit _) -> unit {
            return {};
        });
    }
};

struct Server {
    Reactor reactor;
    std::unordered_map<string, std::unique_ptr<ProcessHandler> > multilinks;
    string target_host;
    int target_port;

    ProcessHandler& get_or_create(string multilink_id) {
        auto& item = multilinks[multilink_id];
        if (!item) {
            LOG("creating new handler process");
            auto on_close = [this, multilink_id]() {
                multilinks.erase(multilink_id);
            };
            item = std::unique_ptr<ProcessHandler>(new ProcessHandler(reactor, on_close, target_host, target_port));
        }

        return *item;
    }

    void run(string listen_host, int listen_port,
             string target_host, int target_port,
             TlsStream::ServerPskFunc identity_callback) {
        Process::init();
        TlsStream::init();

        this->target_port = target_port;
        this->target_host = target_host;

        TCP::listen(reactor, listen_host, listen_port, [&](FD* fd) {
            LOG("incoming connection");
            auto stream = new TlsStream(reactor, fd);
            stream->set_cipher_list("PSK-AES256-CBC-SHA");
            stream->set_psk_server_callback(identity_callback);
            stream->handshake_as_server();

            ioutil::read(stream, 16).then([this, stream](ByteString multilink_id) -> unit {
                // TODO: check identity
                get_or_create(multilink_id).add_link(stream, "link");
                return {};
            }).ignore();
        }).ignore();

        reactor.run();
    }
};

ByteString file_identity_callback(string filename, const char* identity) {
    std::ifstream stream (filename);
    if (!stream) {
        LOG("failed to open identity file " << filename);
        return ByteString(0);
    }

    string line;
    while (std::getline(stream, line)) {
        string prefix = string(identity) + ":";
        if (line.size() > prefix.size() && line.substr(0, prefix.size()) == prefix) {
            string psk = hex_decode(line.substr(prefix.size(), line.size()));
            LOG("identity " << identity << " found");
            return ByteString::copy_from(psk);
        }
    }
    LOG("identity " << identity << " not found");
    return ByteString(0);
}

int main(int argc, char** argv) {
    po::options_description desc ("Start Multilink server");
    desc.add_options()
        ("help", "produce help message")
        ("listen-port,p",
         po::value<int>()->default_value(4500), "port to listen to")
        ("listen-host,h",
         po::value<string>()->default_value("127.0.0.1"), "address to listen to")
        ("target-port,P",
         po::value<int>()->default_value(5000), "address to tunnel connections to")
        ("target-host,H",
         po::value<string>()->default_value("127.0.0.1"), "port to tunnel connections to")
        ("identity-file,f",
         po::value<string>()->default_value("identity_default"), "identity file");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    int listen_port = vm["listen-port"].as<int>();
    string listen_host = vm["listen-host"].as<string>();
    int target_port = vm["target-port"].as<int>();
    string target_host = vm["target-host"].as<string>();
    string identity_file = vm["identity-file"].as<string>();

    Server server;
    server.run(listen_host, listen_port, target_host, target_port,
               std::bind(file_identity_callback, identity_file, std::placeholders::_1));
}
