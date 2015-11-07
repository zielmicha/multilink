import net, socketpair, future

proc pipeSocket(src: Socket, dst: Socket) =
  var buffer = newString(4096)
  while true:
    let recvSize = src.recv(buffer.cstring, buffer.len)
    dst.send(buffer[0..recvSize - 1])

proc startPipeSocket(src: Socket, dst: Socket) =
  var thread: Thread[tuple[src: Socket, dst: Socket]]
  thread.createThread((src, dst) => pipeSocket(src, dst), (src, dst))

proc wrapAsFd*(sock: Socket): cint =
  var fds: array[2, cint]
  if socketpair(posix.AF_UNIX, posix.SOCK_STREAM, 0, fds) < 0:
    raiseOSError(osLastError())

  var wrapped = newSocket(sock, buffered = true)
  startPipeSocket(sock, wrapped)
  startPipeSocket(wrapped, sock)

  return fds[1]
