import app_client
import argparse
import socket
import struct
import os
import threading

class Handler(app_client.HandlerBase):
    def __init__(self, sock_path, connect_addrs, listen_addr):
        self.sock_path = sock_path
        self.ctl = app_client.Connection(self.sock_path)
        self.connect_addrs = connect_addrs
        self.listen_addr = listen_addr
        self.lock = threading.Lock()
        self.multilink_counter = 0
        self.stream_counter = 0

    def create_connection(self, ident, bind, addr):
        s = socket.socket()
        s.bind(bind)
        s.connect(addr)

        f = s.makefile('r+', 1)
        f.write(struct.pack('!I', len(ident)) + ident)
        f.flush()
        return f

    def handle_child(self, child, child_addr):
        ident = os.urandom(16).encode('hex')
        print 'handle connection', ident

        m_id = self.make_multilink(child.makefile('r+'))

        for bind, addr in self.connect_addrs:
            conn = self.create_connection(ident, bind, addr)
            name = 'client@%s' % bind[0]
            conn_fd = self.provide_stream(conn, name)

            with self.lock:
                self.ctl.add_link(m_id, conn_fd, name)

    def run(self):
        server = socket.socket()
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        server.bind(self.listen_addr)
        server.listen(5)
        while True:
            child, addr = server.accept()
            t = threading.Thread(target=self.handle_child, args=[child, addr])
            t.daemon = True
            t.start()
            del child

    def make_multilink(self, stream):
        m_id = self.multilink_counter
        self.multilink_counter += 1

        stream_fd = self.provide_stream(stream, 'client internal')

        self.ctl.make_multilink(m_id, stream_fd, free=True)
        return m_id

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('sock')
    parser.add_argument('listen_port', type=int)
    parser.add_argument('connect_addrs', nargs='+')
    args = parser.parse_args()

    def parse_addr(s):
        bind, addr, port = s.split(':')
        return ((bind, 0), (addr, int(port)))

    connect_addrs = [
        parse_addr(addr) for addr in args.connect_addrs ]

    Handler(args.sock, connect_addrs,
            ('localhost', args.listen_port)).run()
