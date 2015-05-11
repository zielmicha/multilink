import app_client
import argparse
import socket
import threading
import struct

class Handler(app_client.HandlerBase):
    def __init__(self, sock_path, target_addr, src_addr):
        self.target_addr = target_addr
        self.src_addr = src_addr
        self.sock_path = sock_path

        self.multilink_counter = 0
        self.stream_counter = 0
        self.multilinks = {}

        self.lock = threading.RLock()
        self.ctl = app_client.Connection(sock_path)

    def run(self):
        sock = socket.socket()
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        sock.bind(self.src_addr)
        sock.listen(5)

        while True:
            child, addr = sock.accept()
            print 'connection from', addr
            t = threading.Thread(target=self.handle_child, args=[child, addr])
            t.daemon = True
            t.start()
            del child

    def handle_child(self, sock, addr):
        f = sock.makefile('r+', 1)
        id_length, = struct.unpack('!I', f.read(4))
        assert id_length < 4096
        id = f.read(id_length)

        with self.lock:
            if id not in self.multilinks:
                self.make_multilink(id)

            self.add_link(id, sock, 'link %s' % (addr, ))

    def make_multilink(self, id):
        m_id = self.multilink_counter
        self.multilinks[id] = m_id
        self.multilink_counter += 1

        print 'create multilink id=%s bound to %s' % (m_id, self.target_addr)
        stream = socket.create_connection(self.target_addr).makefile('r+')
        stream_fd = self.provide_stream(stream, 'server internal')

        self.ctl.make_multilink(m_id, stream_fd, free=True)

    def add_link(self, id, sock, name):
        m_id = self.multilinks[id]
        stream_fd = self.provide_stream(sock.makefile('r+'), 'server conn')
        self.ctl.add_link(m_id, stream_fd, name)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('sock')
    parser.add_argument('target_port', type=int)
    parser.add_argument('src_port', type=int)

    args = parser.parse_args()

    Handler(args.sock, ('localhost', args.target_port),
            ('0.0.0.0', args.src_port)).run()
