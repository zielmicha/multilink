cmake_minimum_required (VERSION 2.6)
project(Link)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -D_GLIBCXX_DEBUG -fcolor-diagnostics -Wno-unused-private-field")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lrt")

include_directories(${CMAKE_SOURCE_DIR}/libreactor ${CMAKE_SOURCE_DIR}/multilink)

set(COMMON_SRC
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
  libreactor/throttled.cpp)

add_library(reactor STATIC ${COMMON_SRC})

add_library(multilink STATIC
  multilink/multilink.cpp
  multilink/packet_queue.cpp
  multilink/packet_stream_util.cpp
  multilink/write_queue.cpp
  multilink/reorder_stream.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)

include(tests/CMakeLists.txt)
