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

const std::string CHILD_PATH = "./build/server_child";

struct ProcessHandler {
    Reactor& reactor;
    string path;
    ProcessPtr process;

    PacketStreamPtr main_stream;
    int stream_counter = 0;
    int multilink_num = 0;

    std::function<void()> on_close;

    ProcessHandler(Reactor& reactor, std::function<void()> on_close,
                   std::vector<string> child_options):
        reactor(reactor), on_close(on_close) {
        path = "/tmp/.ml_" + random_hex_string(32);

        FDPtr server_socket = UnixSocket::bind(reactor, path);
        server_socket->set_close_on_exec(false);
        std::vector<string> args =
            {CHILD_PATH, "--sock-fd", std::to_string(server_socket->fileno())};
        for (auto opt : child_options) args.push_back(opt);

        process = Popen(reactor, args).exec();
        server_socket->close();

        main_stream = LengthPacketStream::create(reactor, UnixSocket::connect(reactor, path));
        main_stream->set_on_send_ready(nothing);
        main_stream->set_on_recv_ready(nothing);
        multilink_num = 0;
    }

    ProcessHandler(const ProcessHandler& other) = delete;

    Future<unit> send_message(PacketStreamPtr stream, Json value) {
        return ioutil::send(stream, ByteString::copy_from(value.dump()));
    }

    void add_link(StreamPtr stream, string name) {
        auto raw_stream = UnixSocket::connect(reactor, path);
        auto control_stream = LengthPacketStream::create(reactor, raw_stream);

        control_stream->set_on_send_ready(nothing);
        send_message(control_stream, Json::object {
                { "type", "add-link" },
                { "name", name }
            }).ignore();

        ioutil::read(raw_stream, 3).then([=](ByteString s) -> unit {
            if (string(s) != "ok\n") {
                LOG("unexpected response from app: " << s);
                abort();
            }
            pipe_both(reactor, stream, raw_stream, Multilink::LINK_MTU);
            return {};
        }).ignore();
    }
};

struct Server {
    Reactor reactor;
    std::unordered_map<string, std::unique_ptr<ProcessHandler> > multilinks;
    std::vector<string> child_options;

    ProcessHandler& get_or_create(string multilink_id) {
        auto& item = multilinks[multilink_id];
        if (!item) {
            LOG("creating new handler process");
            auto on_close = [this, multilink_id]() {
                multilinks.erase(multilink_id);
            };
            item = std::unique_ptr<ProcessHandler>(new ProcessHandler(reactor, on_close, child_options));
        }

        return *item;
    }

    void run(string listen_host, int listen_port, std::vector<string> child_options,
             TlsStream::ServerPskFunc identity_callback) {
        this->child_options = child_options;

        TCP::listen(reactor, listen_host, listen_port, [&](FDPtr fd) {
            LOG("incoming connection");
            auto stream = std::make_shared<TlsStream>(reactor, fd);
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

    void verify_child_options(std::vector<string> options) {
        options.insert(options.begin(), CHILD_PATH);
        options.push_back("--dry-run");
        Popen(reactor, options).check_call().wait(reactor);
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
    TlsStream::init();
    Process::init();

    po::options_description desc ("Start Multilink server");
    desc.add_options()
        ("help", "produce help message")
        ("listen-port,p",
         po::value<int>()->default_value(4500), "port to listen to")
        ("listen-host,h",
         po::value<string>()->default_value("127.0.0.1"), "address to listen to")
        ("child-option,x",
         po::value<std::vector<string> >(), "option for the child process")
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
    string identity_file = vm["identity-file"].as<string>();
    std::vector<string> child_options;
    if (vm.count("child-option"))
        child_options = vm["child-option"].as<std::vector<string> >();

    Server server;
    server.verify_child_options(child_options);
    server.run(listen_host, listen_port, child_options,
               std::bind(file_identity_callback, identity_file, std::placeholders::_1));
}
