import socket
import threading
import time
import struct

s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('0.0.0.0', 9500))
s.listen(2)

def wr(ch):
    remote_time, = struct.unpack('<Q', ch.makefile('r').read(8))
    time_delta = remote_time - int(time.time() * 1000 * 1000)
    while True:
        data = struct.pack('<Q', time_delta + int((time.time()) * 1000 * 1000)) * 50
        ch.sendall(data)

while True:
    ch, _ = s.accept()
    threading.Thread(target=wr, args=[ch]).start()
    del ch
