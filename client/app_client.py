import socket
import struct
import json
import threading
import time
import sys
import passfd
import os
import subprocess
import functools

def find_app():
    if os.path.exists('./app'):
        return './app'
    else:
        return '../build/app'

def spawn_app():
    path = find_app()
    sock_path = '/tmp/.app_%s' % os.urandom(16).encode('hex')
    proc = subprocess.Popen([path, sock_path])

    while proc.poll() is None:
        if os.path.exists(sock_path):
            break

        time.sleep(0.1)

    return sock_path, proc

class Connection(object):
    def __init__(self, path='../build/app.sock'):
        self.sock = socket.socket(socket.AF_UNIX)
        self.sock.connect(path)
        self.file = self.sock.makefile('r+')

    def send(self, msg):
        msg = json.dumps(msg)
        self.sock.sendall(struct.pack('I', len(msg)) + msg)

    def provide_stream(self, num):
        self.send({'type': 'provide-stream', 'num': num})
        resp = self.file.read(3)
        if resp != 'ok\n':
            raise IOError('bad response to provide-stream')
        return self.file

    def pass_stream(self, num, fd):
        self.send({'type': 'pass-stream', 'num': num})
        passfd.sendfd(self.sock, fd)

        resp = self.file.read(3)
        if resp != 'ok\n':
            raise IOError('bad response to pass-stream')

    def make_multilink(self, num, stream_fd, free=False):
        self.send({'type': 'multilink',
                   'num': num, 'stream_fd': stream_fd,
                   'free': free})

    def make_multilink_with_transport(self, num, addr, is_server):
        self.send({'type': 'multilink-client-server', 'num': num,
                   'host': addr[0], 'port': int(addr[1]), 'is_server': is_server})

    def add_link(self, num, stream_fd, name):
        self.send({'type': 'add-link', 'name': name,
                   'num': num, 'stream_fd': stream_fd})

    def limit_stream(self, stream_fd, buffsize, delay, mbps):
        self.send({'type': 'limit-stream', 'stream_fd': stream_fd,
                   'buffsize': buffsize, 'mbps': mbps,
                   'delay': delay})

class LengthPacketStream(object):
    def __init__(self, f):
        self.f = f

    def recv(self):
        length, = struct.unpack('I', self.f.read(4))
        return self.f.read(length)

    def send(self, data):
        self.f.write(struct.pack('I', len(data)))
        self.f.write(data)
        self.f.flush()

def pipe(label, a, b):
    next_msg = [0]
    count = [0]

    def do():
        while True:
            d = a._sock.recv(4096)
            if not d:
                break

            b.write(d)
            b.flush()

            count[0] += len(d)
            if time.time() > next_msg[0]:
                sys.stdout.write('%s receiving (%d bytes)\n' % (label, count[0]))
                next_msg[0] = time.time() + 0.1

    t = threading.Thread(target=do)
    t.daemon = True
    t.start()

def pipe2(label, a, b):
    pipe(label + ' a->b', a, b)
    pipe(label + ' b->a', b, a)

class HandlerBase(object):
    def __init__(self, sock_path):
        self._exitproc = None
        self.app = None

        if not sock_path:
            self.sock_path, self.app = spawn_app()
            self._exitproc = functools.partial(os.unlink, self.sock_path)

        self.ctl = Connection(self.sock_path)

    def __del__(self):
        if self._exitproc:
            self._exitproc()

    def provide_stream(self, stream, name='provide'):
        id = self.stream_counter
        conn = Connection(self.sock_path)
        conn.pass_stream(id, stream.fileno())
        self.stream_counter += 1
        return id
