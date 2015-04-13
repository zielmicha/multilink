import socket
import struct
import json

s = socket.socket(socket.AF_UNIX)
s.connect('build/app.sock')
msg = json.dumps({'hello': 'world'})
s.sendall(struct.pack('I', len(msg)) + msg)
