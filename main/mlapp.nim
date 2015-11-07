import mlcppthread, json

type
  App = ref object
    serverSock: MulitlinkSocket

proc loop(app: App) =
  let mySock = app.serverSock.connect()
  # TOOD: mySock.writeMessage(%*{"type": "wait-for-reconnects"})
  while true:
    let msg = mySock.readMessage()

proc addClientLink(app: App, listenAddr: string) =
  nil

proc addLinkFd(app: App, fd: int) =
  let sock = app.serverSock.connect()
  sock.writeMessage(%*{"type": ""})

proc main(app: App) =
  app.serverSock = startMultilink()
  app.loop()

when isMainModule:
  var app = new(App)
  app.main()
