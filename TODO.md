
* better transport congestion control (SSH-like)
* refactor Stream* to std::shared_ptr<Stream>
* Futures should print warning if they are destoryed unconsumed
  * + use `__attribute__((warn_unused_result))`
