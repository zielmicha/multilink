
* better transport congestion control (SSH-like)
* terminate: mark lwIP connections as accepted after server returns ok
* Futures should print warning if they are destoryed unconsumed
  * + use `__attribute__((warn_unused_result))`
* detect when server is restarted and drop existing connections
* gracefully handle situation when there are no more FDs left
