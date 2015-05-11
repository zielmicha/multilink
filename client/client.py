import app_client
import argparse
import socket
import struct
import os
import threading

class Handler(app_client.HandlerBase):
    def __init__(self, sock_path, connect_addr, listen_addr):
        self.sock_path = sock_path
        self.ctl = app_client.Connection(self.sock_path)
        self.connect_addr = connect_addr
        self.listen_addr = listen_addr
        self.lock = threading.Lock()
        self.multilink_counter = 0
        self.stream_counter = 0

    def create_connection(self, ident, bind, addr):
        s = socket.socket()
        s.bind((bind, 0))
        s.connect(addr)

        f = s.makefile('r+', 1)
        f.write(struct.pack('!I', len(ident)) + ident)
        return f

    def handle_child(self, child, addr):
        print 'handle connection'
        ident = os.urandom(16).encode('hex')
        conn = self.create_connection(ident, '0.0.0.0', self.connect_addr)
        conn_fd = self.provide_stream(conn, 'client conn')

        with self.lock:
            m_id = self.make_multilink(child.makefile('r+'))
            self.ctl.add_link(m_id, conn_fd, 'client')

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
    parser.add_argument('connect_port', type=int)
    parser.add_argument('listen_port', type=int)
    args = parser.parse_args()

    Handler(args.sock,
            ('localhost', args.connect_port),
            ('localhost', args.listen_port)).run()
