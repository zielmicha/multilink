cmake_minimum_required (VERSION 2.6)
project(Link)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -O3 -Wall -I${CMAKE_SOURCE_DIR}/libreactor -I${CMAKE_SOURCE_DIR}/multilink")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lcrafter")

set(COMMON_SRC
  libreactor/reactor.cpp
  libreactor/common.cpp
  libreactor/epoll.cpp
  libreactor/logging.cpp
  libreactor/signals.cpp
  libreactor/process.cpp
  libreactor/tcp.cpp)

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
