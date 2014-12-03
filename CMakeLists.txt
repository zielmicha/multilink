cmake_minimum_required (VERSION 2.6)
project(Link)

set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -D_GLIBCXX_DEBUG")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lcrafter")

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
  libreactor/buffer.cpp)

add_library(reactor STATIC ${COMMON_SRC})

add_executable(ReactorTest tests/reactor_test.cpp)
target_link_libraries(ReactorTest reactor)

add_executable(ProcessTest tests/process_test.cpp)
target_link_libraries(ProcessTest reactor)

add_executable(TCPTest tests/tcp_test.cpp)
target_link_libraries(TCPTest reactor)

add_executable(TunTest multilink/craftertun.cpp
  tests/craftertest.cpp)
target_link_libraries(TunTest crafter reactor)

add_executable(MultiLinkTest
  tests/multilink_test.cpp
  multilink/multilink.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)
target_link_libraries(MultiLinkTest reactor)

add_executable(LinkTest
  tests/link_test.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)
target_link_libraries(LinkTest reactor)
