#define LOGGER_NAME "lwip_tcp"
#include "libreactor/logging.h"
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

const int DEVICE_MTU = 1500; // TODO: think about this

class NetworkInterfaceImpl {
    netif iface;
    AllocBuffer output_buffer;

    void output(IpAddress ip, pbuf* p);
public:
    NetworkInterfaceImpl();
    ~NetworkInterfaceImpl();

    std::function<void(IpAddress, Buffer)> on_send_packet;

    void on_recv(const Buffer buffer);

    static NetworkInterfaceImpl* get(netif* n) {
        return (NetworkInterfaceImpl*) n->state;
    }
};

NetworkInterface::NetworkInterface() {
    lwip_init();
    impl = new NetworkInterfaceImpl();
}

NetworkInterface::~NetworkInterface() {
    delete impl;
}

void NetworkInterface::set_on_send_packet(std::function<void(IpAddress, Buffer)> callback) {
    impl->on_send_packet = callback;
}

void NetworkInterface::on_recv(const Buffer data) {
    impl->on_recv(data);
}

// ---- NetworkInterfaceImpl

NetworkInterfaceImpl::NetworkInterfaceImpl(): output_buffer(DEVICE_MTU + 4) {
    ip_addr_t addr;
    addr.addr = inet_addr("10.77.0.2");
    ip_addr_t netmask;
    netmask.addr = inet_addr("255.255.255.0");
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
        // Passes packet from netif to the TCP driver
        if (p->len == 0) {
            ERROR("empty packet");
            pbuf_free(p);
            return ERR_OK;
        }

        AllocBuffer buf (p->tot_len);
        pbuf_copy_partial(p, buf.as_buffer().data, buf.as_buffer().size, 0);

        // extract IP version
        int version = (((uint8_t*) p->payload)[0] >> 4);

        if (version == 4)
            return ip_input(p, in);
        else if (version == 6)
            return ip6_input(p, in);
        else
            LOG("unknown IP version " << version);

        pbuf_free(p);
        return ERR_OK;
    };

    bool ok = netif_add(&iface, &addr, &netmask, &gw, (void*) this, netif_init_func, netif_input_func);
    assert (ok);

    netif_set_up(&iface);
    netif_set_pretend_tcp(&iface, 1);
    netif_set_default(&iface);
}

void NetworkInterfaceImpl::output(IpAddress ip, pbuf* p) {
    if (p->tot_len > DEVICE_MTU) {
        ERROR("lwIP tried to output packet > DEVICE_MTU");
        return;
    }

    Buffer buf = output_buffer.as_buffer().slice(0, p->tot_len + 4);

    // prepare headers for TUN
    buf.convert<uint16_t>(0) = 0;
    buf.convert<uint16_t>(2) =
        htons(ip.type == IpAddress::v4 ? 0x0800 : 0x86DD);

    Buffer data_buf = buf.slice(4, p->tot_len);
    pbuf_copy_partial(p, data_buf.data, data_buf.size, 0);

    on_send_packet(ip, buf);
}

void NetworkInterfaceImpl::on_recv(const Buffer data) {
    if (data.size > DEVICE_MTU) {
        LOG("packet too big (" << data.size << ")");
        return;
    }

    const Buffer body = data.slice(4);
    pbuf* p = pbuf_alloc(PBUF_IP, body.size, PBUF_RAM);
    memcpy(p->payload, body.data, body.size);
    iface.input(p, &iface);
}

NetworkInterfaceImpl::~NetworkInterfaceImpl() {
    netif_remove(&iface);
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
    on_accept(std::make_shared<TcpStream>(new_pcb));
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

IpAddress TcpStream::get_remote_address() {
    if(PCB_ISIPV6(pcb)) {
        return IpAddress::make6(pcb->remote_ip.ip6.addr);
    } else {
        return IpAddress::make4(pcb->remote_ip.ip4.addr);
    }
}

uint16_t TcpStream::get_remote_port() {
    return pcb->remote_port;
}

IpAddress TcpStream::get_local_address() {
    if(PCB_ISIPV6(pcb)) {
        return IpAddress::make6(pcb->local_ip.ip6.addr);
    } else {
        return IpAddress::make4(pcb->local_ip.ip4.addr);
    }
}

uint16_t TcpStream::get_local_port() {
    return pcb->local_port;
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
