# coding=utf-8
import socket
import struct
import json
import threading
import time
import collections
import os
import sys

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

ctl = Connection()

def add_pair(k, delay, buffsize, mbps):
    t0 = Connection().provide_stream(k + 0)
    t1 = Connection().provide_stream(k + 1)

    pipe(k, t0, t1)
    pipe(k + 1, t1, t0)

    ctl.limit_stream(k, delay=delay, buffsize=buffsize, mbps=mbps)

    ctl.add_link(num=0, stream_fd=k + 0, name=str(k + 0))
    ctl.add_link(num=1, stream_fd=k + 1, name=str(k + 1))

m0 = LengthPacketStream(Connection().provide_stream(0))
m1 = LengthPacketStream(Connection().provide_stream(1))

ctl.make_multilink(num=0, stream_fd=0)
ctl.make_multilink(num=1, stream_fd=1)

add_pair(10,
         delay=1 * 1000, buffsize=1 * 1000 * 1000, mbps=40)

add_pair(20,
         delay=1 * 1000, buffsize=1 * 1000 * 1000, mbps=40)

count = 10000
size = 2048

def send_data():
    for i in xrange(count):
        m0.send(struct.pack('I', i) + ' ' * (size - 4))
        if i % 1000 == 0:
            print 'sent', i

    print 'sent'

t = threading.Thread(target=send_data)
t.daemon = True
t.start()

timeframe = 0.01
collected = collections.defaultdict(int)
start = time.time()

ifound_list = []

for i in xrange(count):
    data = m1.recv()
    assert len(data) == size
    ifound, = struct.unpack('I', data[:4])
    ifound_list.append(ifound)

    if i % 1000 == 0:
        print 'read', i

    t = time.time() - start
    collected[int(t / timeframe)] += size

assert(set(ifound_list) == set(range(count)))

def get_term_width():
    rows, columns = os.popen('stty size', 'r').read().split()
    return int(columns)

def draw_graph(collected):
    term_width = get_term_width()

    tick = '▇'
    max_value = max(collected.values()) * 2

    space = term_width - 20

    unit = max_value / space

    for i in xrange(max(collected.keys()) + 1):
        value = int(collected[i] / unit)
        label = '%d ms %d kBps' % (i * timeframe * 1000, collected[i] / timeframe / 1024)
        print label.ljust(20), tick * value

print 'finish'

draw_graph(collected)
