cmake_minimum_required (VERSION 2.6)
project(Link)

# clang has linking problems on Ubuntu 15.10
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(OPT_COMMON "-fsanitize=address")
# set(OPT_COMMON "")
# for clang++: -Wno-unused-private-field -D_GLIBCXX_DEBUG
set(CMAKE_CXX_FLAGS "-std=c++11 -g -Wall -fdiagnostics-color -Og ${OPT_COMMON} -Wno-literal-suffix")
set(CMAKE_C_FLAGS "-std=c11 -D_GNU_SOURCE")
set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -lrt ${OPT_COMMON}")

find_package (Boost 1.55.0 REQUIRED COMPONENTS program_options)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/deps/json11 ${Boost_INCLUDE_DIR})

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
  libreactor/bytestring.cpp
  libreactor/packet_stream_util.cpp)

add_library(reactor_tls STATIC
  libreactor/tls.cpp)

add_library(multilink STATIC
  multilink/multilink.cpp
  multilink/packet_queue.cpp
  multilink/write_queue.cpp
  multilink/transport.cpp
  multilink/transport_targets.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)

include(deps/lwip/CMakeLists.txt)

add_library(terminate STATIC
  terminate/tun.cpp
  terminate/terminate.cpp
  terminate/lwip_tcp.cpp)

include(tests/CMakeLists.txt)

add_executable(app
  app/app.cpp
  app/rpc.cpp)

target_link_libraries(app multilink reactor json11)

add_executable(multilink_client
  app/client.cpp)

target_link_libraries(multilink_client multilink reactor_tls terminate lwip reactor json11 ssl crypto ${Boost_LIBRARIES})

add_executable(multilink_server
  app/server.cpp)

target_link_libraries(multilink_server reactor_tls reactor json11 ssl crypto ${Boost_LIBRARIES})

add_executable(server_child
  app/server_child.cpp
  app/rpc.cpp)

target_link_libraries(server_child multilink terminate lwip reactor json11 ${Boost_LIBRARIES})
