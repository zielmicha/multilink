cmake_minimum_required (VERSION 2.6)
project(Link)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -Werror -D_GLIBCXX_DEBUG -fcolor-diagnostics -Wno-unused-private-field")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lrt")

include_directories(${CMAKE_SOURCE_DIR}/libreactor ${CMAKE_SOURCE_DIR}/multilink ${CMAKE_SOURCE_DIR}/deps/json11 ${CMAKE_SOURCE_DIR}/app)

add_library(json11 STATIC
  ${CMAKE_SOURCE_DIR}/deps/json11/json11.cpp)

add_library(reactor STATIC
  libreactor/reactor.cpp
  libreactor/common.cpp
  libreactor/epoll.cpp
  libreactor/logging.cpp
  libreactor/signals.cpp
  libreactor/process.cpp
  libreactor/tcp.cpp
  libreactor/misc.cpp
  libreactor/buffer.cpp
  libreactor/timer.cpp
  libreactor/streamutil.cpp
  libreactor/throttled.cpp
  libreactor/unix_socket.cpp
  libreactor/ioutil.cpp
  libreactor/bytestring.cpp)

add_library(multilink STATIC
  multilink/multilink.cpp
  multilink/packet_queue.cpp
  multilink/packet_stream_util.cpp
  multilink/write_queue.cpp
  multilink/reorder_stream.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)

include(tests/CMakeLists.txt)

add_executable(app
  app/app.cpp
  app/rpc.cpp)

target_link_libraries(app multilink reactor json11)
