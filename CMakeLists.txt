cmake_minimum_required (VERSION 2.6)
project(Link)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -O3 -Wall -Ilibreactor")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lcrafter")

set(COMMON_SRC
  libreactor/reactor.cpp
  libreactor/common.cpp
  libreactor/epoll.cpp
  libreactor/logging.cpp
  libreactor/signals.cpp
  libreactor/process.cpp
  libreactor/tcp.cpp)
add_executable(ReactorTest ${COMMON_SRC} reactor_test.cpp)
add_executable(ProcessTest ${COMMON_SRC} process_test.cpp)
add_executable(TCPTest ${COMMON_SRC} tcp_test.cpp)

add_executable(TunTest ${COMMON_SRC} craftertun.cpp craftertest.cpp)
target_link_libraries(TunTest crafter)
