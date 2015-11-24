#include "logging.h"
#include "tcp.h"
#include "tls.h"
#include "ioutil.h"
#include "misc.h"

int main() {
    Reactor reactor;

    TlsStream::init();

    TCP::listen(reactor, "127.0.0.1", 8800, [&](FD* fd) {
        LOG("incoming connection");
        auto stream = new TlsStream(reactor, fd);
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
    }).ignore();

    TCP::connect(reactor, "127.0.0.1", 8800).then([&](FD* fd) -> Future<ByteString> {
        LOG("connected");
        auto stream = new TlsStream(reactor, fd);

        stream->on_write_ready = nothing;
        stream->set_host_name("tls-test.example");
        stream->set_cipher_list("PSK-AES256-CBC-SHA");
        stream->set_psk_client_callback([](const char* hint) {
            LOG("identity hint: " << (hint == NULL ? "(NULL)" : hint));
            return TlsStream::IdentityAndPsk(ByteString::copy_from("michal"),
                                             ByteString::copy_from("1234567890"));
        });
        stream->handshake_as_client();
        return ioutil::read(stream, 4);
    }).then([](ByteString data) {
        LOG("recv " << data);
        return unit();
    });

    reactor.run();
}
