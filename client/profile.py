import socket
import select
import time
import json
import sys

def profile_once(bind):
    s = socket.socket()
    start = time.time()
    s.bind((bind, 0))
    s.connect(('mlserwery', 9500))

    data = []
    recv = 0

    while True:
        now = time.time() - start
        if now > 20:
            break

        r, w, x = select.select([s], [], [])
        if r:
            recv += len(s.recv(4096))

        data.append((now, recv))

    return data

if __name__ == '__main__':
    n = int(sys.argv[2])
    out = open(sys.argv[1], 'w')
    bind = '0.0.0.0' if len(sys.argv) == 3 else sys.argv[3]
    for i in xrange(n):
        print 'Test', i
        data = profile_once(bind)
        out.write(json.dumps(data) + '\n')
        out.flush()
