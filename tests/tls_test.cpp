#include "libreactor/logging.h"
#include "libreactor/tcp.h"
#include "libreactor/tls.h"
#include "libreactor/ioutil.h"
#include "libreactor/misc.h"

int main() {
    Reactor reactor;

    TlsStream::init();

    std::vector<TlsStreamPtr> streams;

    TCP::listen(reactor, "127.0.0.1", 8800, [&](FDPtr fd) {
        LOG("incoming connection");
        auto stream = TlsStream::create(reactor, fd);
        stream->on_read_ready = nothing;
        stream->set_cipher_list("PSK-AES256-CBC-SHA");
        stream->set_psk_identity_hint("marianna");
        stream->set_psk_server_callback([](const char* identity) {
            LOG("identity: " << identity);
            return ByteString::copy_from("1234567890");
        });
        stream->handshake_as_server();
        LOG("before write");
        ioutil::write(stream, ByteString::copy_from("helo"));

        stream->on_read_ready = [stream]() {
            AllocBuffer buffer (4096);
            while (true) {
                Buffer data = stream->read(buffer.as_buffer());
                if (data.size == 0) break;
            }
        };

        streams.push_back(stream);
    }).ignore();

    TlsStreamPtr stream =
        TCP::connect(reactor, "127.0.0.1", 8800).then([&](FDPtr fd) {
            LOG("connected");
            auto stream = TlsStream::create(reactor, fd);

            stream->on_write_ready = nothing;
            stream->set_host_name("tls-test.example");
            stream->set_cipher_list("PSK-AES256-CBC-SHA");
            stream->set_psk_client_callback([](const char* hint) {
                LOG("identity hint: " << (hint == NULL ? "(NULL)" : hint));
                return TlsStream::IdentityAndPsk(ByteString::copy_from("michal"),
                                                 ByteString::copy_from("1234567890"));
            });
            stream->handshake_as_client();

            return stream;
        }).wait(reactor);

    ioutil::read(stream, 4).then([](ByteString data) {
        LOG("recv " << data);
        return unit();
    }).ignore();

    std::function<void()> write;

    ByteString val (4096);
    memset(val.as_buffer().data, 'A', val.size());

    write = [&]() {
        ioutil::write(stream, val).then([&](unit) -> unit {
            LOG ("write");
            write();
            return {};
        }).ignore();
    };

    write();

    reactor.run();
}
