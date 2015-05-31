import app_client
import argparse
import socket
import threading
import struct

class Handler(app_client.HandlerBase):
    def __init__(self, sock_path, target_addr, listen_addr):
        super(Handler, self).__init__(sock_path)

        self.target_addr = target_addr
        self.listen_addr = listen_addr

        # start with 1000, so we can run on one instance with client
        self.multilink_counter = 1000
        self.stream_counter = 1000
        self.multilinks = {}

        self.lock = threading.RLock()

    def run(self):
        sock = socket.socket()
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(self.listen_addr)
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
        assert id_length < 4096, 'bad id_length: %s' % id_length
        id = f.read(id_length)
        print 'starting child:', id

        with self.lock:
            if id not in self.multilinks:
                self.make_multilink(id)

            self.add_link(id, sock, 'link %s' % (addr, ))

    def make_multilink(self, id):
        m_id = self.multilink_counter
        self.multilinks[id] = m_id
        self.multilink_counter += 1

        print 'create multilink id=%s bound to %s' % (m_id, self.target_addr)

        self.ctl.make_multilink_with_transport(m_id, is_server=True, addr=self.target_addr)

    def add_link(self, id, sock, name):
        m_id = self.multilinks[id]
        stream_fd = self.provide_stream(sock.makefile('r+'), 'server conn')
        self.ctl.add_link(m_id, stream_fd, name)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--sock', help='C++ server Unix socket (by default spawn it)')
    parser.add_argument('target_port', type=int, help='Target port')
    parser.add_argument('listen_port', type=int)

    args = parser.parse_args()

    Handler(args.sock, ('127.0.0.1', args.target_port),
            ('0.0.0.0', args.listen_port)).run()
