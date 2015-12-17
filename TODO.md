
* automatically enable forwarding (sysctl net.ipv4.ip_forward=1 )
* better transport congestion control (SSH-like)
  * before: choke/unchoke in transport
* terminate: mark lwIP connections as accepted after server returns ok
* Futures should print warning if they are destoryed unconsumed
  * + use `__attribute__((warn_unused_result))`
* gracefully handle situation when there are no more FDs left
* replace all std::bind(..., this) with std::bind(..., shared_from_this())
* seccomp
* crash still possible after removing (heap-use-after-free)
* removing link seems to cause memory leak (rather large)
* timeout TLS connections
* sleep mode
