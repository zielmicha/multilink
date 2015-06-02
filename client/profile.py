import socket
import select
import time
import json
import sys

def profile_once():
    s = socket.socket()
    start = time.time()
    s.connect(('users.atomshare.net', 9500))

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
    for i in xrange(n):
        print 'Test', i
        data = profile_once()
        out.write(json.dumps(data) + '\n')
        out.flush()
