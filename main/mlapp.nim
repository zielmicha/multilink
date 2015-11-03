import json, net, commonnim, future, posix, os

proc app_main(length: cint, path: cstring) {.importc.}

proc runMultilink(path: string) {.thread.} =
  app_main(path.len.cint, path)

proc main() =
  var fds: array[2, cint]
  if socketpair(posix.AF_UNIX, posix.SOCK_STREAM, 0, fds) < 0:
    raiseOSError(osLastError())

  var thread: Thread[string]
  thread.createThread(runMultilink, "&" & $fds[0])
  thread.joinThread()

proc readMessage(sock: Socket): JsonNode =
  var size: uint32
  if sock.recv(addr size, sizeof size) != sizeof size:
    raise newException(IOError, "EOF")
  var data: string
  if sock.recv(data, size.int) != size.int:
    raise newException(IOError, "EOF")
  return parseJson(data)

when isMainModule:
  main()
