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

class NetworkInterface {
    NetworkInterfaceImpl* impl;
public:
    NetworkInterface();
};

class TcpListener: public std::enable_shared_from_this<TcpListener> {
    tcp_pcb* listener = nullptr;
    //tcp_pcb* listener_ip6;

    void accept(tcp_pcb* new_pcb, err_t err);
public:
    TcpListener();
    ~TcpListener();
};

class TcpStream: public AbstractStream {
    tcp_pcb* pcb;
    void recv_from_pcb(pbuf* buf);
    std::deque<std::pair<pbuf*, int> > recv_queue;
public:
    TcpStream(tcp_pcb* pcb);
    ~TcpStream();

    size_t write(const Buffer data);
    Buffer read(Buffer data);
    void close();
};

#endif
