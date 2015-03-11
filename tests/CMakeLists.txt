add_executable(ReactorTest tests/reactor_test.cpp)
target_link_libraries(ReactorTest reactor)

add_executable(ProcessTest tests/process_test.cpp)
target_link_libraries(ProcessTest reactor)

add_executable(TCPTest tests/tcp_test.cpp)
target_link_libraries(TCPTest reactor)

add_executable(TimerTest tests/timer_test.cpp)
target_link_libraries(TimerTest reactor)

#add_executable(TunTest multilink/craftertun.cpp
#  tests/craftertest.cpp)
#target_link_libraries(TunTest crafter reactor)

add_executable(LinkTest
  tests/link_test.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)
target_link_libraries(LinkTest reactor)

add_executable(LinkBwTest
  tests/link_bw_test.cpp
  multilink/multilink_link.cpp
  multilink/multilink_stats.cpp)
target_link_libraries(LinkBwTest reactor)

add_executable(ThrottledTest
  tests/throttled_test.cpp)
target_link_libraries(ThrottledTest reactor)

add_executable(DelayedTest
  tests/delayed_test.cpp)
target_link_libraries(DelayedTest reactor)

add_executable(PacketQueueTest
  tests/packet_queue_test.cpp
  multilink/packet_queue.cpp)
target_link_libraries(PacketQueueTest reactor)

add_executable(MultiLinkTest
  tests/multilink_test.cpp)
target_link_libraries(MultiLinkTest multilink reactor)

add_executable(ReorderStreamTest
  tests/reorder_stream_test.cpp)
target_link_libraries(ReorderStreamTest multilink reactor)

add_executable(SequenceServer
  tests/sequence_server.cpp)
target_link_libraries(SequenceServer multilink reactor)

add_executable(SequenceClient
  tests/sequence_client.cpp)
target_link_libraries(SequenceClient multilink reactor)

add_executable(MLProxyServer
  tests/ml_proxy_server.cpp)
target_link_libraries(MLProxyServer multilink reactor)

add_executable(MLProxyClient
  tests/ml_proxy_client.cpp)
target_link_libraries(MLProxyClient multilink reactor)

add_executable(PipeTest
  tests/pipe_test.cpp)
target_link_libraries(PipeTest multilink reactor)

add_executable(FreePacketStreamTest
  tests/free_packet_stream_test.cpp)
target_link_libraries(FreePacketStreamTest multilink reactor)

add_executable(WriteQueueTest
  tests/write_queue_test.cpp)
target_link_libraries(WriteQueueTest multilink reactor)

add_executable(MLBandwidthTest
  tests/ml_bandwidth_test.cpp)
target_link_libraries(MLBandwidthTest multilink reactor)