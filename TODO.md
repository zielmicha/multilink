
* better transport congestion control (SSH-like)
* Futures should print warning if they are destoryed unconsumed
  * + use `__attribute__((warn_unused_result))`
* detect when server is restarted and drop existing connections
