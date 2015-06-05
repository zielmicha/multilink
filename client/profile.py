import socket
import select
import time
import json
import sys
import struct

def profile_once(bind):
    s = socket.socket()
    start = time.time()
    s.bind((bind, 0))
    s.connect(('77.55.252.125', 9500))
    begin_time = int(time.time() * 1000 * 1000)
    s.sendall(struct.pack('<Q', begin_time))

    values = []
    recv = 0

    while True:
        now = time.time() - start
        if now > 20:
            break

        r, w, x = select.select([s], [], [])

        if r:
            data = s.recv(4096)
            offset = recv % 8
            real_data = data[offset:]
            recv += len(data)

            field_count = len(real_data) / 8
            if field_count != 0:
                transmit_time, = struct.unpack('<Q', real_data[field_count * 8 - 8:field_count * 8])
                current_time = int(time.time() * 1000 * 1000)
                delta = current_time - transmit_time

                values.append((now, recv, delta))

    return values

if __name__ == '__main__':
    n = int(sys.argv[2])
    out = open(sys.argv[1], 'w')
    bind = '0.0.0.0' if len(sys.argv) == 3 else sys.argv[3]
    for i in xrange(n):
        print 'Test', i
        values = profile_once(bind)
        out.write(json.dumps(values) + '\n')
        out.flush()
