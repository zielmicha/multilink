#ifndef TERMINATE_LWIP_TCP_
#define TERMINATE_LWIP_TCP_
#include <memory>
#include <deque>
#include "libreactor/reactor.h"
#include "terminate/address.h"

struct netif;
struct tcp_pcb;
struct pbuf;
typedef int8_t err_t;
class NetworkInterfaceImpl;

class NetworkInterface;
class TcpStream;
class TcpListener;

typedef std::shared_ptr<NetworkInterface> NetworkInterfacePtr;
typedef std::shared_ptr<TcpStream> TcpStreamPtr;
typedef std::shared_ptr<TcpListener> TcpListenerPtr;

class NetworkInterface {
    NetworkInterfaceImpl* impl;
public:
    NetworkInterface();
    ~NetworkInterface();

    void on_recv(const Buffer data);
    void set_on_send_packet(std::function<void(IpAddress, Buffer)> callback);
};

class TcpListener: public std::enable_shared_from_this<TcpListener> {
    tcp_pcb* listener = nullptr;
    //tcp_pcb* listener_ip6;

    void accept(tcp_pcb* new_pcb, err_t err);
public:
    TcpListener();
    ~TcpListener();

    std::function<void(TcpStreamPtr)> on_accept;
};

class TcpStream: public AbstractStream {
    tcp_pcb* pcb;
    void recv_from_pcb(pbuf* buf);
    std::deque<std::pair<pbuf*, int> > recv_queue;

public:
    TcpStream(tcp_pcb* pcb);
    ~TcpStream();

    IpAddress get_remote_address();
    uint16_t get_remote_port();
    IpAddress get_local_address();
    uint16_t get_local_port();

    size_t write(const Buffer data);
    Buffer read(Buffer data);
    void close();
};

#endif
