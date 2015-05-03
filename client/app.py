import socket
import struct
import json
import threading

class Connection(object):
    def __init__(self):
        self.sock = socket.socket(socket.AF_UNIX)
        self.sock.connect('build/app.sock')
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

    def make_multilink(self, num, stream_fd):
        self.send({'type': 'multilink',
                   'num': num, 'stream_fd': stream_fd})

    def add_link(self, num, stream_fd, name):
        self.send({'type': 'add-link', 'name': name,
                   'num': num, 'stream_fd': stream_fd})

class LengthPacketStream(object):
    def __init__(self, f):
        self.f = f

    def read(self):
        length, = struct.unpack('I', self.f.read(4))
        return self.f.read(length)

    def write(self, data):
        self.f.write(struct.pack('I', len(data)))
        self.f.write(data)
        self.f.flush()

def pipe(a, b):
    def do():
        while True:
            d = a._sock.recv(4096)
            if not d:
                break
            b.write(d)

    t = threading.Thread(target=do)
    # t.daemon = True
    t.start()

ctl = Connection()
t0 = Connection().provide_stream(10)
t1 = Connection().provide_stream(11)
pipe(t0, t1)
pipe(t1, t0)

m0 = LengthPacketStream(Connection().provide_stream(0))
m1 = LengthPacketStream(Connection().provide_stream(1))

ctl.make_multilink(num=0, stream_fd=0)
ctl.make_multilink(num=1, stream_fd=1)

ctl.add_link(num=0, stream_fd=10, name='10')
ctl.add_link(num=1, stream_fd=11, name='11')

m0.write('Foobar')
print m1.read()
