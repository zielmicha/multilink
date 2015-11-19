#include "tls.h"
#define LOGGER_NAME "tls"
#include "logging.h"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

static SSL_CTX* ssl_context;
static int tls_stream_index;

const int MTU = 8192;
const int MAX_BIO_BUFFER = 32768;

TlsStream::TlsStream(Reactor& reactor, Stream* stream): reactor(reactor),
                                                        stream(stream),
                                                        out_buffer(MTU) {
    // FIXME: we do excessive amount of buffering
    packet_stream = FreePacketStream::create(reactor, stream, MTU);

    ssl = SSL_new(ssl_context);
    assert(ssl);

    SSL_set_ex_data(ssl, tls_stream_index, (void*) this);

    bio_in = BIO_new(BIO_s_mem());
    bio_out = BIO_new(BIO_s_mem());

    SSL_set_bio(ssl, bio_in, bio_out);

    // SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);

    packet_stream->set_on_send_ready(std::bind(&TlsStream::transport_send_ready, this));
    packet_stream->set_on_recv_ready(std::bind(&TlsStream::transport_recv_ready, this));
    packet_stream->set_on_error([this]() {
        this->on_error();
    });
}

TlsStream::~TlsStream() {
    if (ssl)
        SSL_free(ssl);
}


void TlsStream::close() {
    stream->close();
}


void TlsStream::transport_send_ready() {
    while (packet_stream->is_send_ready()) {
        int pending = BIO_ctrl_pending(bio_out);
        if (pending <= 0) {
            on_write_ready();
            break;
        }

        int amount = BIO_read(bio_out,
                              out_buffer.data, std::min((int) out_buffer.size, pending));
        assert (amount > 0);

        Buffer buffer = out_buffer.as_buffer().slice(0, amount);
        packet_stream->send(buffer);
    }
}

void TlsStream::transport_recv_ready() {
    while (BIO_ctrl_wpending(bio_in) < MAX_BIO_BUFFER) {
        optional<Buffer> buffer = packet_stream->recv();
        if (!buffer) break;

        int ret = BIO_write(bio_in, buffer->data, buffer->size);
        assert(ret >= 0);
    }

    on_read_ready();
}

void tls_error(const char* name) {
    auto err = ERR_peek_last_error();
    auto err_str = ERR_error_string(err, NULL);
    LOG(name << ": OpenSSL error: " << err_str);
}

size_t TlsStream::write(const Buffer buffer) {
    int ret = SSL_write(ssl, buffer.data, buffer.size);

    if (ret > 0) {
        assert(ret == buffer.size);
        return ret;
    }

    int err = SSL_get_error(ssl, ret);

    if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
        return 0;

    tls_error("TlsStream::write");
    reactor.schedule(on_error);
    return 0;
}

Buffer TlsStream::read(Buffer out) {
    int pointer = 0;
    while (true) {
        int ret = SSL_read(ssl, out.data + pointer, out.size - pointer);

        if (ret < 0) {
            int err = SSL_get_error(ssl, ret);
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
                break;

            tls_error("TlsStream::read");
            reactor.schedule(on_error);
            return out.slice(0, 0);
        }
        if (ret == 0)
            break;

        pointer += ret;
    }

    return out.slice(0, pointer);
}

void TlsStream::handshake_as_server() {
    SSL_set_accept_state(ssl);
}

void TlsStream::handshake_as_client() {
    SSL_set_connect_state(ssl);
}

void TlsStream::set_cipher_list(const char* list) {
    SSL_set_cipher_list(ssl, list);
}

void TlsStream::set_psk_identity_hint(const char* hint) {
    SSL_use_psk_identity_hint(ssl, hint);
}

unsigned TlsStream::psk_client_callback(SSL *ssl, const char *hint,
                                        char *identity_out, unsigned int max_identity_len,
                                        unsigned char *psk_out, unsigned int max_psk_len) {

    TlsStream* self = (TlsStream*)SSL_get_ex_data(ssl, tls_stream_index);
    IdentityAndPsk ret = self->client_psk_func(hint);

    Buffer psk = ret.psk.as_buffer();
    Buffer identity = ret.identity.as_buffer();

    assert (psk.size <= max_psk_len);
    assert (identity.size < max_identity_len);

    memcpy(psk_out, psk.data, psk.size);
    memcpy(identity_out, identity.data, identity.size);
    identity_out[identity.size] = 0;

    return psk.size;
}

void TlsStream::set_psk_client_callback(ClientPskFunc func) {
    client_psk_func = func;
    SSL_set_psk_client_callback(ssl, &TlsStream::psk_client_callback);
}

unsigned TlsStream::psk_server_callback(SSL *ssl, const char *identity,
                                        unsigned char *psk_out, unsigned int max_psk_len) {
    TlsStream* self = (TlsStream*)SSL_get_ex_data(ssl, tls_stream_index);
    ByteString psk_b = self->server_psk_func(identity);
    Buffer psk = psk_b.as_buffer();

    assert (psk.size <= max_psk_len);
    memcpy(psk_out, psk.data, psk.size);
    return psk.size;
}

void TlsStream::set_psk_server_callback(ServerPskFunc func) {
    server_psk_func = func;
    SSL_set_psk_server_callback(ssl, &TlsStream::psk_server_callback);
}

void TlsStream::init() {
    CRYPTO_malloc_init();
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    ssl_context = SSL_CTX_new(SSLv23_method()); // TODO: use TLSv1_2_method()

    assert(ssl_context);

    tls_stream_index = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
}
