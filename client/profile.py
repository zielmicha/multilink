import socket
import select
import time
import json
import sys
import struct

def profile_once(bind, conn):
    s = socket.socket()
    start = time.time()
    s.bind((bind, 0))
    s.connect((conn, 9500))
    begin_time = int(time.time() * 1000 * 1000)
    s.sendall(struct.pack('<Q', begin_time))

    values = []
    recv = 0
    last_info = 0

    while True:
        now = time.time() - start
        if now > 60:
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

                current_time = time.time()
                if current_time - last_info > 1:
                    last_info = current_time
                    print 'Latency %d ms Recv %d MB' % (delta / 1000, recv / 1024 / 1024)

    return values

if __name__ == '__main__':
    n = int(sys.argv[2])
    out = open(sys.argv[1], 'w')
    bind = '0.0.0.0' if len(sys.argv) <= 3 else sys.argv[3]
    conn = '127.0.0.1' if len(sys.argv) <= 4 else sys.argv[4]
    for i in xrange(n):
        print 'Test', i
        values = profile_once(bind, conn)
        out.write(json.dumps(values) + '\n')
        out.flush()
        print 'Sleeping after test...'
        time.sleep(6)
