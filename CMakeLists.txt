cmake_minimum_required (VERSION 2.6)
project(Link)

set(CMAKE_CXX_FLAGS "-std=c++11 -g -O3 -Wall")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lcrafter")

set(COMMON_SRC reactor.cpp common.cpp epoll.cpp logging.cpp signals.cpp process.cpp tcp.cpp)
add_executable(ReactorTest ${COMMON_SRC} reactor_test.cpp)
add_executable(ProcessTest ${COMMON_SRC} process_test.cpp)
add_executable(TCPTest ${COMMON_SRC} tcp_test.cpp)

add_executable(TunTest ${COMMON_SRC} craftertun.cpp craftertest.cpp)
target_link_libraries(TunTest crafter)
