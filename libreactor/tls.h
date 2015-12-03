#ifndef TLS_H_
#define TLS_H_
#include "libreactor/reactor.h"
#include "libreactor/bytestring.h"
#include "libreactor/packet_stream_util.h"

struct ssl_st;
struct bio_t;
typedef struct ssl_st SSL;
typedef struct bio_st BIO;

class TlsStream;

typedef TlsStream* TlsStreamPtr;

class TlsStream : public AbstractStream {
    Reactor& reactor;
    StreamPtr stream;

    std::shared_ptr<FreePacketStream> packet_stream;
    AllocBuffer out_buffer;

    SSL* ssl;
    BIO* bio_in;
    BIO* bio_out;

    void transport_send_ready();
    void transport_recv_ready();
    bool handle_ssl_want(int err);

    static unsigned psk_client_callback(SSL *ssl, const char *hint,
                                        char *identity, unsigned int max_identity_len,
                                        unsigned char *psk, unsigned int max_psk_len);
    static unsigned psk_server_callback(SSL *ssl, const char *identity,
                                   unsigned char *psk, unsigned int max_psk_len);

public:
    struct IdentityAndPsk {
        IdentityAndPsk(ByteString identity, ByteString psk): identity(identity), psk(psk) {}
        ByteString identity;
        ByteString psk;
    };

    using ClientPskFunc = std::function<IdentityAndPsk(const char* hint)>;
    using ServerPskFunc = std::function<ByteString(const char* identity)>;

private:

    ClientPskFunc client_psk_func;
    ServerPskFunc server_psk_func;

public:

    TlsStream(Reactor& reactor, StreamPtr stream);
    ~TlsStream();

    void handshake_as_server();
    void handshake_as_client();
    void set_host_name(const char* name);
    void set_cipher_list(const char* list);
    void set_psk_identity_hint(const char* hint);

    void set_psk_client_callback(ClientPskFunc client_psk_func);
    void set_psk_server_callback(ServerPskFunc server_psk_func);

    Buffer read(Buffer buffer);
    size_t write(const Buffer buffer);
    void close();

    static void init();
};

#endif
