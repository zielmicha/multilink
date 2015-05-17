# coding=utf-8
import struct
import threading
import time
import collections
import os

from app_client import Connection, LengthPacketStream, pipe

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

add_pair(10, delay=1 * 1000, buffsize=2 * 1000 * 1000, mbps=30000)
add_pair(20, delay=100 * 1000, buffsize=2 * 1000 * 1000, mbps=50000)

count = 30000
size = 2048
K = 1000

def send_data():
    for i in xrange(count):
        m0.send(struct.pack('I', i) + ' ' * (size - 4))
        if i % K == 0:
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

    if i % K == 0:
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

bps = count * size / (max(collected.keys()) * timeframe)
print 'MBps:', bps / 1000.0 / 1000
