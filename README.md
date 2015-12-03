# Multilink

Multilink is an implementation of my [stream-based aggregation of unreliable heterogeneous network links](http://arxiv.org/abs/1509.08222) paper. That is, it allows you to create one stable and fast connection based on mutliple crappier links. It is not usable yet, but it soon will be.

## Build

1. Checkout submodules

  ```
  git submodule update --init --recursive
  ```

2. Install dependencies

  ```
  apt-get install libboost-program-options-dev libboost-filesystem-dev ninja-build cmake
  ```

  or equivalent for your distro.

3. Configure

  ```
  mkdir build && (cd build && cmake -GNinja ..)
  ```

4. Build

  ```
  ninja -C build
  ```

  If build fails with linker error mentioning `boost::program_options`, it may be a problem with your compiler version.

## Bundled libraries

* lwip (`deps/lwip`) - with modifications from BadVPN. The `lwip_tcp.cpp` is inspired by the BadVPN *tun2socks*.
* json11 (`deps/json11`) - JSON manipulation librarary from Dropbox.
