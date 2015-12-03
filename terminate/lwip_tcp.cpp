#include <netinet/in.h>
#include <arpa/inet.h>
#include "terminate/lwip_tcp.h"

extern "C" {
#include <lwip/init.h>
#include <lwip/tcp_impl.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
}

class NetworkInterfaceImpl {
    netif iface;
    void output(IpAddress ip, pbuf* p);
public:
    NetworkInterfaceImpl();

    static NetworkInterfaceImpl* get(netif* n) {
        return (NetworkInterfaceImpl*)(((char *)n) - offsetof(NetworkInterfaceImpl, iface));
    }
};

NetworkInterfaceImpl::NetworkInterfaceImpl() {
    ip_addr_t addr;
    addr.addr = inet_addr("10.155.0.2");
    ip_addr_t netmask;
    netmask.addr = inet_addr("255.255.255.255");
    ip_addr_t gw;
    ip_addr_set_any(&gw);

    auto netif_init_func = [](netif* n) -> err_t {
        n->name[0] = 'h';
        n->name[1] = 'o';
        n->output = [](netif* n, pbuf* p, ip_addr_t* ipaddr) -> err_t {
            NetworkInterfaceImpl::get(n)->output(IpAddress::make4(ipaddr->addr), p);
            return ERR_OK;
        };
        n->output_ip6 = [](netif* n, pbuf* p, ip6_addr_t* ipaddr) -> err_t {
            NetworkInterfaceImpl::get(n)->output(IpAddress::make6(ipaddr->addr), p);
            return ERR_OK;
        };

        return ERR_OK;
    };

    auto netif_input_func = [](pbuf* p, netif* in) -> err_t {
        return ERR_OK;
    };

    bool ok = netif_add(&iface, &addr, &netmask, &gw, NULL, netif_init_func, netif_input_func);
    assert (ok);

    netif_set_up(&iface);
    netif_set_pretend_tcp(&iface, 1);
    netif_set_default(&iface);

}

NetworkInterface::NetworkInterface() {
    lwip_init();
    impl = new NetworkInterfaceImpl();
}

TcpListener::TcpListener() {
    struct tcp_pcb *l = tcp_new();
    assert(l);

    int err = tcp_bind_to_netif(l, "ho0");
    assert(err == ERR_OK);

    listener = tcp_listen(l);
    assert(listener);

    tcp_arg(listener, (void*) this);
    tcp_accept(listener, [](void* arg, tcp_pcb* new_pcb, err_t err) -> err_t {
        ((TcpListener*) arg)->accept(new_pcb, err);
        return ERR_OK;
    });
}

void TcpListener::accept(tcp_pcb* new_pcb, err_t err) {
    tcp_accepted(listener);
    //TcpStream(new_pcb)...
}

TcpListener::~TcpListener() {
    if(listener) tcp_close(listener);
}

TcpStream::TcpStream(tcp_pcb* pcb): pcb(pcb) {
    tcp_arg(pcb, this);
    tcp_err(pcb, [](void* args, err_t err) {
        ((TcpStream*) args)->on_error();
    });
    tcp_recv(pcb, [](void* arg, tcp_pcb* pcb, pbuf* buf, err_t err) -> err_t {
        assert(err == ERR_OK);
        ((TcpStream*) arg)->recv_from_pcb(buf);
        return ERR_OK;
    });
    tcp_sent(pcb, [](void* arg, tcp_pcb* pcb, u16_t len) -> err_t {
        ((TcpStream*) arg)->on_write_ready();
        return ERR_OK;
    });
}

void TcpStream::recv_from_pcb(pbuf* buf) {
    bool was_empty = recv_queue.empty();
    recv_queue.push_back({buf, 0});

    if (was_empty)
        on_read_ready();
}

Buffer TcpStream::read(Buffer out) {
    if (recv_queue.empty())
        return Buffer(nullptr, 0);

    pbuf* p = recv_queue.front().first;
    int& iter = recv_queue.front().second;
    int read_size = std::min((int) out.size, p->tot_len - iter);

    pbuf_copy_partial(p, out.data, read_size, iter);
    iter += read_size;

    if (iter == p->tot_len) {
        recv_queue.pop_front();
        pbuf_free(p);
    }

    tcp_recved(pcb, read_size);
    return out.slice(0, read_size);
}

void TcpStream::close() {
    if (pcb) {
        tcp_close(pcb);
        pcb = nullptr;
    }
}

size_t TcpStream::write(const Buffer data) {
    size_t size = std::min(data.size, (size_t) tcp_sndbuf(pcb));

    err_t err = tcp_write(pcb, data.data, size, TCP_WRITE_FLAG_COPY);
    assert (err == ERR_OK);
    return size;
}

TcpStream::~TcpStream() {
    if (pcb) tcp_close(pcb);
}
