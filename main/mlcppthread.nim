import json, net, commonnim, future, os

proc app_main(length: cint, path: cstring) {.importc.}

type MulitlinkSocket* = distinct string
  ## A connection to the Multilink C++ app thread.

proc runMultilink(path: string) {.thread.} =
  app_main(path.len.cint, path)

proc startMultilink*(): MulitlinkSocket =
  var thread: Thread[string]
  let path = "/tmp/.mlapp_" & hexUrandom(16)

  let serverSock = newSocket(AF_UNIX, SOCK_STREAM, IPPROTO_IP)
  serverSock.bindUnix(path)
  setFilePermissions(path, {fpUserRead, fpUserWrite})
  serverSock.listen()

  thread.createThread(runMultilink, "&" & serverSock.getFd().cint.`$`)

  return path.MulitlinkSocket

proc connect*(socket: MulitlinkSocket): Socket =
  result = newSocket(AF_UNIX, SOCK_STREAM, IPPROTO_IP)
  result.connectUnix(socket.string)

proc readMessage*(sock: Socket): JsonNode =
  var size: uint32
  if sock.Socket.recv(addr size, sizeof size) != sizeof size:
    raise newException(IOError, "EOF")
  var data: string
  if sock.Socket.recv(data, size.int) != size.int:
    raise newException(IOError, "EOF")
  return parseJson(data)

proc writeMessage*(sock: Socket, message: JsonNode) =
  var data = message.`$`
  var len: uint32 = data.len.uint32
  var lenStr = newString(4)
  copyMem(lenStr.cstring, addr len, 4)
  sock.send(lenStr & data, flags = {})
