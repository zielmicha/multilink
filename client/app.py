import socket
import struct
import json

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

conn = Connection()
f = conn.provide_stream(0)
print f
